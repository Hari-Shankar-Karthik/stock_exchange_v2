#pragma once

#include "book/order_book.hpp"
#include "core/constants.hpp"
#include "core/order.hpp"
#include "utils/log.hpp"
#include "utils/spsc_queue.hpp"

namespace exchange::matching
{
    using book::OrderBook;
    using core::EVENT_BUFFER_SIZE;
    using core::INCOMING_BUFFER_SIZE;
    using core::MAX_LEVELS;
    using core::Order;
    using core::REPLAY_LOG_PAGES;
    using core::TradeEvent;
    using utils::Log;
    using utils::SPSCQueue;

    class MatchingEngine
    {
    public:
        MatchingEngine(SPSCQueue<Order, INCOMING_BUFFER_SIZE> &_incoming, Log<Order, REPLAY_LOG_PAGES> &_log, SPSCQueue<TradeEvent, EVENT_BUFFER_SIZE> &_tradeEvents, SPSCQueue<TradeEvent, EVENT_BUFFER_SIZE> &_marketData) noexcept
            : incoming(_incoming), log(_log), book(_tradeEvents, _marketData)
        {
            // TODO: When there are multiple symbols, maintain an array mapping from Symbol to OrderBook.
        }

        void run() noexcept
        {
            Order o;
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
        SPSCQueue<Order, INCOMING_BUFFER_SIZE> &incoming;
        Log<Order, REPLAY_LOG_PAGES> &log;
        std::uint64_t currSequenceNumber{0};
        OrderBook book; // for now, only dealing with a single symbol
    };
}