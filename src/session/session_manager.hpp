#pragma once

#include <asio.hpp>
#include <cstddef>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <utility>

#include "core/constants.hpp"
#include "core/order.hpp"
#include "core/trade_event.hpp"
#include "core/types.hpp"
#include "utils/slab_allocator.hpp"
#include "utils/spsc_queue.hpp"

namespace exchange::session
{
    using exchange::core::ClientId;
    using exchange::core::EVENT_BUFFER_SIZE;
    using exchange::core::INCOMING_BUFFER_SIZE;
    using exchange::core::MAX_CLIENTS;
    using exchange::core::Order;
    using exchange::core::TradeEvent;
    using utils::SlabAllocator;
    using utils::SPSCQueue;

    // template <typename K, typename V, std::size_t N>
    // using SlabUnorderedMap = std::unordered_map<K, V, std::hash<K>, std::equal_to<K>, SlabAllocator<std::pair<const K, V>, N>>;

    class SessionManager
    {
    public:
        SessionManager(SPSCQueue<Order, INCOMING_BUFFER_SIZE> &_incoming, SPSCQueue<TradeEvent, EVENT_BUFFER_SIZE> &_outgoing, const std::string_view serverIpAddr, const uint16_t serverPort)
            : incoming(_incoming), context(), acceptor(context), serverEndpoint(asio::ip::make_address(serverIpAddr), serverPort), outgoing(_outgoing)
        {
            acceptor.open(serverEndpoint.protocol());
            acceptor.set_option(asio::socket_base::reuse_address(true));

            acceptor.bind(serverEndpoint);
            acceptor.listen(); // TODO: What should the backlog be?
        }
        ~SessionManager() = default;

        SessionManager(const SessionManager &other) = delete;
        SessionManager(SessionManager &&other) = delete;

        SessionManager &operator=(const SessionManager &other) = delete;
        SessionManager &operator=(SessionManager &&other) = delete;

        void receiver()
        {
            acceptConnection();
            while (true)
            {
                context.poll();
            }
        }

        void sender()
        {
            TradeEvent e;
            while (true)
            {
                if (!outgoing.pop(e))
                {
                    continue;
                }

                for (const auto clientId : {e.buyerId, e.sellerId})
                {
                    int fd;
                    {
                        std::lock_guard<std::mutex> lg(mtx);
                        if (!fds.contains(clientId))
                        {
                            continue;
                        }
                        fd = fds[clientId];
                    }

                    // This should be non-blocking (sleeps if the send buffer is full)
                    // TODO: Queue up the unsent messages for when the buffer clears
                    // TODO: Kick the client based on the number of unsent messages
                    ::send(fd, &e, sizeof(e), 0);
                }
            }
        }

    private:
        // Receiver-only
        uint32_t numClients{0};
        ClientId currClientId{0};
        SlabAllocator<asio::ip::tcp::socket, MAX_CLIENTS> socketAllocator;
        SlabAllocator<Order, MAX_CLIENTS> messageAllocator;
        SPSCQueue<Order, INCOMING_BUFFER_SIZE> &incoming;
        asio::io_context context;
        asio::ip::tcp::acceptor acceptor;
        asio::ip::tcp::endpoint serverEndpoint;

        // Shared between sender and receiver
        alignas(64) std::mutex mtx;
        std::unordered_map<ClientId, int> fds; // TODO: This flies against the zero-allocation policy

        // Sender-only
        alignas(64) SPSCQueue<TradeEvent, EVENT_BUFFER_SIZE> &outgoing;

        void acceptConnection()
        {
            acceptor.async_accept(
                [this](const asio::error_code &ec, asio::ip::tcp::socket socket)
                {
                    if (!ec)
                    {
                        socket.set_option(asio::ip::tcp::no_delay(true)); // Prevent bandwidth-saving delays
                        ClientId clientId = ++currClientId;

                        {
                            int fd = socket.native_handle();
                            std::lock_guard<std::mutex> lg(mtx);
                            fds[clientId] = fd;
                        }

                        auto socketPtr = socketAllocator.allocate(1);
                        auto messagePtr = messageAllocator.allocate(1);
                        std::construct_at(socketPtr, std::move(socket));

                        if (++numClients < MAX_CLIENTS)
                        {
                            acceptConnection();
                        }
                        readMessage(socketPtr, messagePtr, clientId);
                    }
                });
        }

        void readMessage(asio::ip::tcp::socket *socketPtr, Order *messagePtr, ClientId clientId)
        {
            asio::async_read(
                *socketPtr,
                asio::buffer(messagePtr, sizeof(Order)),
                [this, socketPtr, messagePtr, clientId](const asio::error_code &ec, std::size_t bytesRead)
                {
                    // TODO: Be more robust by using bytesRead
                    if (!ec)
                    {
                        messagePtr->clientId = clientId;
                        incoming.push(*messagePtr);
                        readMessage(socketPtr, messagePtr, clientId);
                    }
                    else
                    {
                        // This client has closed their connection
                        {
                            std::lock_guard<std::mutex> lg(mtx);
                            fds.erase(clientId);
                        }

                        socketAllocator.deallocate(socketPtr, 1);
                        messageAllocator.deallocate(messagePtr, 1);
                    }
                });
        }
    };
}
