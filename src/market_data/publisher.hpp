#pragma once

#include <asio.hpp>

#include "config/config.hpp"
#include "core/trade_event.hpp"
#include "helper/types.hpp"
#include "sync/sync_variables.hpp"

namespace exchange::market_data
{
    using config::MULTICAST_IP_ADDR;
    using config::MULTICAST_PORT;
    using core::TradeEvent;
    using helper::TradeQueue;
    using sync::isProcessingClosed;

    class MarketDataPublisher
    {
    public:
        MarketDataPublisher(TradeQueue &_marketData)
            : context(1), socket(context), marketData(_marketData)
        {
            asio::ip::udp::endpoint multicastEndpoint(asio::ip::make_address(MULTICAST_IP_ADDR), MULTICAST_PORT);
            socket.open(multicastEndpoint.protocol());
            socket.connect(multicastEndpoint);
        }

        void run()
        {
            TradeEvent e;
            while (!isProcessingClosed.load(std::memory_order_acquire))
            {
                if (!marketData.pop(e))
                {
                    continue;
                }

                // Blocking send because this is server-limited
                // TODO: Diagnostics on wait times to warn about increasing buffer limits
                socket.send(asio::buffer(&e, sizeof(e)));
            }
        }

    private:
        asio::io_context context;
        asio::ip::udp::socket socket;
        TradeQueue &marketData;
    };
}