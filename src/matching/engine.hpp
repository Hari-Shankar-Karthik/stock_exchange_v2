#pragma once

#include "core/order.hpp"
#include "book/order_book.hpp"
#include "helper/types.hpp"
#include "sync/sync_variables.hpp"

namespace exchange::matching
{
    using namespace helper;
    using book::OrderBook;
    using sync::isIncomingClosed;
    using sync::isProcessingClosed;

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
            while (!isIncomingClosed.load(std::memory_order_acquire))
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

            isProcessingClosed.store(true, std::memory_order_release);
        }

    private:
        // thee queues lives in global memory
        OrderQueue &incoming;
        ReplayLog &log;
        std::uint64_t currSequenceNumber{0};
        OrderBook book; // for now, only dealing with a single symbol
    };
}