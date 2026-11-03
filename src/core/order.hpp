#pragma once

#include <iostream>
#include <type_traits>

#include "core/types.hpp"

namespace exchange::core
{
    struct alignas(64) Order
    {
        core::OrderType orderType;
        core::Side side;
        core::SequenceNumber sequenceNumber; // set by the exchange
        core::ClientId clientId;             // set by the exchange
        core::Price price;                   // only relevant for LIMIT orders
        core::Quantity quantity;
    };

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
    static_assert(sizeof(Order) == alignof(Order), "Order may straddle cache lines");
    static_assert(sizeof(Order) <= 64U, "Order does not fit in a cache line");
}