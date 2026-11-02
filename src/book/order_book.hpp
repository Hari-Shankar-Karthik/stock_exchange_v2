#pragma once

#include <iostream> // TODO: Temporary
#include <map>

#include "book/price_level.hpp"
#include "core/constants.hpp"
#include "core/order.hpp"
#include "core/trade_event.hpp"
#include "core/types.hpp"
#include "utils/slab_allocator.hpp"
#include "utils/spsc_queue.hpp"

namespace exchange::book
{
    using core::ClientId;
    using core::EVENT_BUFFER_SIZE;
    using core::MAX_LEVELS;
    using core::MAX_RESTING_ORDERS;
    using core::Order;
    using core::Price;
    using core::Quantity;
    using core::Side;
    using core::TradeEvent;
    using utils::SlabAllocator;
    using utils::SPSCQueue;

    template <typename K, typename V, typename Compare, std::size_t N>
    using SlabMap = std::map<K, V, Compare, SlabAllocator<std::pair<const K, V>, N>>;

    class OrderBook
    {
    public:
        OrderBook(SPSCQueue<TradeEvent, EVENT_BUFFER_SIZE> &_tradeEvents,
                  SPSCQueue<TradeEvent, EVENT_BUFFER_SIZE> &_marketData) noexcept
            : tradeEvents(_tradeEvents), marketData(_marketData)
        {
        }

        void process(Order o) noexcept
        {
            std::cerr << "[ENGINE] Processing " << o << std::endl;
            if (o.side == Side::BUY)
            {
                for (auto it = askHalf.begin(); it != askHalf.end() && it->first <= o.price && o.quantity;)
                {
                    RestingOrder *&head = it->second.head;
                    while (head && o.quantity)
                    {
                        if (o.quantity >= head->o.quantity)
                        {
                            publishTrade(o.clientId, head->o.clientId, head->o.price, head->o.quantity);
                            o.quantity -= head->o.quantity;
                            const auto newHead = head->next;
                            restingOrderAllocator.deallocate(head, 1);
                            head = newHead;
                        }
                        else
                        {
                            publishTrade(o.clientId, head->o.clientId, head->o.price, o.quantity);
                            head->o.quantity -= o.quantity;
                            o.quantity = 0;
                        }
                    }
                    if (!head)
                    {
                        it = askHalf.erase(it);
                    }
                    else
                    {
                        ++it;
                    }
                }
                if (o.quantity)
                {
                    RestingOrder *ptr = restingOrderAllocator.allocate(1);
                    ptr->o = o;
                    ptr->next = nullptr;
                    bidHalf[o.price].insert(ptr);
                }
            }
            else
            {
                for (auto it = bidHalf.begin(); it != bidHalf.end() && it->first >= o.price && o.quantity;)
                {
                    RestingOrder *&head = it->second.head;
                    while (head && o.quantity)
                    {
                        if (o.quantity >= head->o.quantity)
                        {
                            publishTrade(head->o.clientId, o.clientId, head->o.price, head->o.quantity);
                            o.quantity -= head->o.quantity;
                            const auto newHead = head->next;
                            restingOrderAllocator.deallocate(head, 1);
                            head = newHead;
                        }
                        else
                        {
                            publishTrade(head->o.clientId, o.clientId, head->o.price, o.quantity);
                            head->o.quantity -= o.quantity;
                            o.quantity = 0;
                        }
                    }
                    if (!head)
                    {
                        it = bidHalf.erase(it);
                    }
                    else
                    {
                        ++it;
                    }
                }
                if (o.quantity)
                {
                    RestingOrder *ptr = restingOrderAllocator.allocate(1);
                    ptr->o = o;
                    ptr->next = nullptr;
                    askHalf[o.price].insert(ptr);
                }
            }
        }

    private:
        // Events represent individual fill reports, market data is broadcast to all subscribers
        SPSCQueue<TradeEvent, EVENT_BUFFER_SIZE> &tradeEvents;
        SPSCQueue<TradeEvent, EVENT_BUFFER_SIZE> &marketData;
        // TODO: [META] Maybe both maps should share an allocator? Better cache locality?
        SlabMap<Price, PriceLevel, std::less<Price>, MAX_LEVELS> askHalf;
        SlabMap<Price, PriceLevel, std::greater<Price>, MAX_LEVELS> bidHalf;
        // TODO: [FUTURE] Have a common allocator across symbols for better cache locality.
        SlabAllocator<RestingOrder, MAX_RESTING_ORDERS> restingOrderAllocator;

        void publishTrade(const ClientId buyerId, const ClientId sellerId, const Price price, const Quantity quantity) noexcept
        {
            TradeEvent e{buyerId, sellerId, price, quantity};
            tradeEvents.push(e);
            marketData.push(e);
        }
    };
}