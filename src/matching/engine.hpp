#pragma once

#include "book/order_book.hpp"
#include "core/constants.hpp"
#include "core/order.hpp"
#include "utils/spsc_queue.hpp"

namespace exchange::matching
{
    using book::OrderBook;
    using core::EVENT_BUFFER_SIZE;
    using core::INCOMING_BUFFER_SIZE;
    using core::LOG_BUFFER_SIZE;
    using core::MAX_LEVELS;
    using core::Order;
    using core::TradeEvent;
    using utils::SPSCQueue;

    class MatchingEngine
    {
    public:
        MatchingEngine(SPSCQueue<Order, INCOMING_BUFFER_SIZE> &_incoming, SPSCQueue<Order, LOG_BUFFER_SIZE> &_log, SPSCQueue<TradeEvent, EVENT_BUFFER_SIZE> &_tradeEvents, SPSCQueue<TradeEvent, EVENT_BUFFER_SIZE> &_marketData) noexcept
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

                // sequence it
                o.sequenceNumber = ++sequenceNumber;

                // log it
                log.push(o);

                // process it
                book.process(o);
            }
        }

    private:
        // thee queues lives in global memory
        SPSCQueue<Order, INCOMING_BUFFER_SIZE> &incoming;
        SPSCQueue<Order, LOG_BUFFER_SIZE> &log;
        std::uint64_t sequenceNumber{0};
        OrderBook book; // for now, only dealing with a single symbol
    };
}