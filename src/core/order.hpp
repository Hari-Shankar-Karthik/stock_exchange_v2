#pragma once

#include <iostream>
#include <type_traits>

#include "core/types.hpp"

namespace exchange::core
{
    struct alignas(32) Order
    {
        core::SequenceNumber sequenceNumber; // to be set by the exchange
        core::ClientId clientId;
        core::Price price;
        core::Quantity quantity;
        core::Side side;
    };

    inline std::ostream &operator<<(std::ostream &os, const Order &o)
    {
        os << "Order(sequenceNumber=" << o.sequenceNumber
           << ", clientId=" << o.clientId
           << ", price=" << o.price
           << ", quantity=" << o.quantity
           << ", side=" << o.side << ")";
        return os;
    }

    static_assert(std::is_trivially_default_constructible_v<Order>);
    static_assert(std::is_standard_layout_v<Order>);
    static_assert(std::is_trivially_copyable_v<Order>);
    static_assert(sizeof(Order) == alignof(Order), "Order may straddle cache lines");
    static_assert(sizeof(Order) <= 64U, "Order does not fit in a cache line");
}