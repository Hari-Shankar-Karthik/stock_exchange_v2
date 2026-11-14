#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <map>
#include <optional>
#include <vector>

#include "core/order.hpp"
#include "core/trade_event.hpp"
#include "core/types.hpp"

namespace exchange::test
{
    // Deliberately naive model of the order book: two flat vectors and linear scans.
    // Written from the matching rules, not from OrderBook's code:
    //   - a trade happens at the RESTING order's price
    //   - best price first (lowest ask / highest bid), earliest arrival first within a price
    //   - LIMIT: unfilled remainder rests. MARKET and IOC: unfilled remainder is discarded.
    //   - MARKET ignores the price field. IOC and LIMIT only match at prices no worse than their limit.
    //   - no self-trade prevention (matches the engine's current TODO)
    class ReferenceBook
    {
    public:
        using Trades = std::vector<core::TradeEvent>;

        struct Snapshot
        {
            std::map<core::Price, core::Quantity> bids; // price -> total resting quantity
            std::map<core::Price, core::Quantity> asks;
            bool operator==(const Snapshot &) const = default;
        };

        Trades process(core::Order o)
        {
            Trades trades;
            const bool incomingIsBuy = o.side == core::Side::BUY;
            std::vector<Resting> &opposite = incomingIsBuy ? asks : bids;
            std::vector<Resting> &same = incomingIsBuy ? bids : asks;

            core::Price limit = o.price;
            if (o.orderType == core::OrderType::MARKET)
            {
                limit = incomingIsBuy ? std::numeric_limits<core::Price>::max()
                                      : std::numeric_limits<core::Price>::min();
            }

            while (o.quantity > 0)
            {
                const auto best = bestIndex(opposite, incomingIsBuy);
                if (!best)
                {
                    break;
                }
                Resting &r = opposite[*best];
                const bool crosses = incomingIsBuy ? r.price <= limit : r.price >= limit;
                if (!crosses)
                {
                    break;
                }

                const core::Quantity qty = std::min(o.quantity, r.quantity);
                const core::ClientId buyer = incomingIsBuy ? o.clientId : r.clientId;
                const core::ClientId seller = incomingIsBuy ? r.clientId : o.clientId;
                trades.push_back(core::TradeEvent{++lastExecId, buyer, seller, r.price, qty});

                o.quantity -= qty;
                r.quantity -= qty;
                if (r.quantity == 0)
                {
                    opposite.erase(opposite.begin() + static_cast<std::ptrdiff_t>(*best));
                }
            }

            if (o.quantity > 0 && o.orderType == core::OrderType::LIMIT)
            {
                same.push_back(Resting{o.clientId, o.price, o.quantity, ++lastArrival});
            }
            return trades;
        }

        Snapshot snapshot() const
        {
            Snapshot s;
            for (const Resting &r : bids)
            {
                s.bids[r.price] += r.quantity;
            }
            for (const Resting &r : asks)
            {
                s.asks[r.price] += r.quantity;
            }
            return s;
        }

        std::size_t restingCount() const { return bids.size() + asks.size(); }

    private:
        struct Resting
        {
            core::ClientId clientId;
            core::Price price;
            core::Quantity quantity;
            std::uint64_t arrival;
        };

        // The resting order that an incoming order of the given side would trade against first.
        static std::optional<std::size_t> bestIndex(const std::vector<Resting> &side, bool incomingIsBuy)
        {
            std::optional<std::size_t> best;
            for (std::size_t i = 0; i < side.size(); ++i)
            {
                if (!best)
                {
                    best = i;
                    continue;
                }
                const Resting &cand = side[i];
                const Resting &cur = side[*best];
                const bool betterPrice = incomingIsBuy ? cand.price < cur.price : cand.price > cur.price;
                const bool earlierAtSamePrice = cand.price == cur.price && cand.arrival < cur.arrival;
                if (betterPrice || earlierAtSamePrice)
                {
                    best = i;
                }
            }
            return best;
        }

        std::vector<Resting> bids;
        std::vector<Resting> asks;
        core::ExecutionId lastExecId{0};
        std::uint64_t lastArrival{0};
    };
}