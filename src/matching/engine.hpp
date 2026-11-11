#pragma once

#include "core/order.hpp"
#include "book/order_book.hpp"
#include "helper/types.hpp"

namespace exchange::matching
{
    using namespace helper;
    using book::OrderBook;

    class MatchingEngine
    {
    public:
        MatchingEngine(OrderQueue &_incoming, ReplayLog &_log, TradeQueue &_tradeEvents, TradeQueue &_marketData) noexcept
            : incoming(_incoming), log(_log), book(_tradeEvents, _marketData)
        {
            // TODO: When there are multiple symbols, maintain an array mapping from Symbol to OrderBook.
        }

        void run() noexcept
        {
            AlignedOrder o;
            while (true)
            {
                if (!incoming.pop(o))
                {
                    continue;
                }

                // TODO: Pre-trade risk/other checks?

                // sequence it
                o.sequenceNumber = ++currSequenceNumber;

                // log it
                log.push(o);

                // process it
                book.process(o);
            }
        }

    private:
        // thee queues lives in global memory
        OrderQueue &incoming;
        ReplayLog &log;
        std::uint64_t currSequenceNumber{0};
        OrderBook book; // for now, only dealing with a single symbol
    };
}