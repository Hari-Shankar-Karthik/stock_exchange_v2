#pragma once

#include "core/order.hpp"

namespace exchange::book
{
    using core::Order;

    struct alignas(64) RestingOrder
    {
        RestingOrder(const Order o_) : o(o_), next(nullptr) {}

        Order o;
        RestingOrder *next;
    };

    static_assert(sizeof(RestingOrder) == alignof(RestingOrder), "Resting order may straddle cache lines");
    static_assert(sizeof(RestingOrder) <= 64U, "Order does not fit in a cache line");
}