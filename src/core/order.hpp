#pragma once

#include <iostream>
#include <type_traits>

#include "core/types.hpp"
#include "utils/cache_aligned.hpp"

namespace exchange::core
{
    struct Order
    {
        OrderType orderType;
        Side side;
        SequenceNumber sequenceNumber; // set by the exchange
        ClientId clientId;             // set by the exchange
        Price price;                   // only relevant for LIMIT orders
        Quantity quantity;
    };

    using utils::CacheAligned;
    using AlignedOrder = CacheAligned<Order>;

    inline std::ostream &operator<<(std::ostream &os, const Order &o)
    {
        os << "Order(orderType=" << o.orderType
           << ", side=" << o.side
           << ", sequenceNumber=" << o.sequenceNumber
           << ", clientId=" << o.clientId
           << ", price=" << o.price
           << ", quantity=" << o.quantity << ")";

        return os;
    }

    static_assert(std::is_trivially_default_constructible_v<Order>);
    static_assert(std::is_standard_layout_v<Order>);
    static_assert(std::is_trivially_copyable_v<Order>);
    static_assert(sizeof(AlignedOrder) == alignof(AlignedOrder), "Order may straddle cache lines");
    static_assert(sizeof(AlignedOrder) <= 64U, "Order does not fit in a cache line");
}