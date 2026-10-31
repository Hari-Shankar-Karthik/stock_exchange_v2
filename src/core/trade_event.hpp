#pragma once

#include <type_traits>

#include "core/types.hpp"

namespace exchange::core
{
    struct alignas(32) TradeEvent
    {
        // TODO: Self-trade protection
        ClientId buyerId;
        ClientId sellerId;
        Price price;
        Quantity quantity;
    };

    static_assert(sizeof(TradeEvent) == 32);
    static_assert(alignof(TradeEvent) == 32);
    static_assert(std::is_trivially_default_constructible_v<TradeEvent>);
    static_assert(std::is_trivially_copyable_v<TradeEvent>);
    static_assert(std::is_trivially_copyable_v<TradeEvent>);
}