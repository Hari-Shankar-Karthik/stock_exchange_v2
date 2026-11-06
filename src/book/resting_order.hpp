#pragma once

#include "core/order.hpp"
#include "core/types.hpp"

namespace exchange::book
{
    using core::ClientId;
    using core::Order;
    using core::Quantity;

    struct alignas(32) RestingOrder
    {
        RestingOrder(const Order o_) : clientId(o_.clientId), quantity(o_.quantity), next(nullptr) {}

        ClientId clientId;
        Quantity quantity;
        RestingOrder *next;
    };

    static_assert(sizeof(RestingOrder) == alignof(RestingOrder), "Resting order may straddle cache lines");
    static_assert(sizeof(RestingOrder) <= 64U, "Resting order does not fit in a cache line");
}