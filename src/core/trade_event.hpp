#pragma once

#include <iostream>
#include <type_traits>

#include "core/types.hpp"

namespace exchange::core
{
    struct alignas(64) TradeEvent
    {
        // TODO: Self-trade protection
        ExecutionId execId;
        ClientId buyerId;
        ClientId sellerId;
        Price price;
        Quantity quantity;
    };

    inline std::ostream &operator<<(std::ostream &os, const TradeEvent &e) noexcept
    {
        os << "TradeEvent(execId=" << e.execId
           << ", buyerId=" << e.buyerId
           << ", sellerId=" << e.sellerId
           << ", price=" << e.price
           << ", quantity=" << e.quantity << ")";
        return os;
    }

    static_assert(sizeof(TradeEvent) == 64);
    static_assert(alignof(TradeEvent) == 64);
    static_assert(std::is_trivially_default_constructible_v<TradeEvent>);
    static_assert(std::is_trivially_copyable_v<TradeEvent>);
    static_assert(std::is_trivially_copyable_v<TradeEvent>);
}