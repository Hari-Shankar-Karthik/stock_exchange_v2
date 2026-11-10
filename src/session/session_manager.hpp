#pragma once

#include <asio.hpp>
#include <cstddef>
#include <memory>
#include <mutex>
#include <unordered_map>

namespace exchange::session
{
    using core::ClientId;
    using core::EVENT_BUFFER_SIZE;
    using core::INCOMING_BUFFER_SIZE;
    using core::MAX_CLIENTS;
    using core::Order;
    using core::TradeEvent;
    using utils::SlabAllocator;
    using utils::SPSCQueue;

    class SessionManager
    {
    public:
        SessionManager(SPSCQueue<Order, INCOMING_BUFFER_SIZE> &_incoming,
                       SPSCQueue<TradeEvent, EVENT_BUFFER_SIZE> &_outgoing,
                       const std::string_view serverIpAddr,
                       const std::uint16_t serverPort)
            : incoming(_incoming), context(1), acceptor(context), outgoing(_outgoing)
        {
            // TODO: Is concurrency_hint == 1 is OK here?
            asio::ip::tcp::endpoint serverEndpoint(asio::ip::make_address(serverIpAddr), serverPort);
            acceptor.open(serverEndpoint.protocol());
            acceptor.set_option(asio::socket_base::reuse_address(true)); // TODO: Is this debug only?

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
                    std::lock_guard lg(mtx);
                    auto it = fds.find(clientId);
                    if (it == fds.end())
                    {
                        continue;
                    }
                    int fd = it->second;

                    // Send while holding the lock to prevent this client from disconnecting right now
                    if (::send(fd, &e, sizeof(e), MSG_DONTWAIT | MSG_NOSIGNAL) < static_cast<ssize_t>(sizeof(e)))
                    {
                        // Message was partially copied/not copied at all into the send buffer
                        // Client will be kicked for not ACKing quickly enough
                        // TODO: Queue up the unsent messages for when the buffer clears and kick based on queue size
                        // All disconnects will be handled by the receiver thread only
                        asio::post(
                            context,
                            [this, clientId]()
                            {
                                disconnectClient(clientId);
                            });
                    }
                }
            }
        }

    private:
        // TODO: Look at field ordering
        // Receiver-only
        std::uint32_t numClients{0};
        ClientId currClientId{0};
        SlabAllocator<asio::ip::tcp::socket, MAX_CLIENTS> socketAllocator;
        SlabAllocator<Order, MAX_CLIENTS> messageAllocator;
        SPSCQueue<Order, INCOMING_BUFFER_SIZE> &incoming;
        asio::io_context context;
        asio::ip::tcp::acceptor acceptor;
        std::unordered_map<int, std::pair<asio::ip::tcp::socket *, Order *>> slots;

        // Shared between sender and receiver
        // TODO: There has to be a better way to ensure no false sharing
        alignas(64) std::mutex mtx;
        std::unordered_map<ClientId, int> fds; // TODO: Use the improved SlabAllocator for this

        // Sender-only
        alignas(64) SPSCQueue<TradeEvent, EVENT_BUFFER_SIZE> &outgoing;

        void acceptConnection()
        {
            acceptor.async_accept(
                [this](const asio::error_code &ec, asio::ip::tcp::socket socket)
                {
                    if (!ec)
                    {
                        socket.set_option(asio::ip::tcp::no_delay(true));
                        ClientId clientId = ++currClientId;

                        auto socketPtr = socketAllocator.allocate(1);
                        auto messagePtr = messageAllocator.allocate(1);
                        std::construct_at(socketPtr, std::move(socket));

                        int fd = socketPtr->native_handle();
                        {
                            std::lock_guard lg(mtx);
                            fds[clientId] = fd;
                        }
                        slots[fd] = std::make_pair(socketPtr, messagePtr);

                        if (++numClients < MAX_CLIENTS)
                        {
                            acceptConnection();
                        }
                        readMessage(socketPtr, messagePtr, clientId);
                    }
                });
        }

        void readMessage(asio::ip::tcp::socket *socketPtr, Order *messagePtr, const ClientId clientId)
        {
            asio::async_read(
                *socketPtr,
                asio::buffer(messagePtr, sizeof(Order)),
                [this, socketPtr, messagePtr, clientId](const asio::error_code &ec, std::size_t bytesRead)
                {
                    // If a disconnect was issued while waiting, the read would have been cancelled
                    // The disconnect may occur after the read but before the handler
                    // Don't have to lock because the only concurrent access (by the sender) is read-only
                    if (!fds.contains(clientId))
                    {
                        return;
                    }

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
                        disconnectClient(clientId);
                    }
                });
        }

        void disconnectClient(const ClientId clientId)
        {
            int fd;
            {
                std::lock_guard lg(mtx);
                auto it = fds.find(clientId);
                if (it == fds.end())
                {
                    return;
                }
                fd = it->second;
                fds.erase(it);
            }

            auto [socketPtr, messagePtr] = slots[fd];
            slots.erase(fd);
            std::destroy_at(socketPtr);
            socketAllocator.deallocate(socketPtr, 1);
            messageAllocator.deallocate(messagePtr, 1);

            if (numClients-- == MAX_CLIENTS)
            {
                // Restart new connections in the receiver thread
                acceptConnection();
            }
        }
    };
}
