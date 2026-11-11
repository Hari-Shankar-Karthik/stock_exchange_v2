#pragma once

#include "core/order.hpp"
#include "core/types.hpp"
#include "utils/cache_aligned.hpp"

namespace exchange::book
{
    using core::ClientId;
    using core::Order;
    using core::Quantity;
    using utils::CacheAligned;

    struct RestingOrder
    {
        RestingOrder(const Order o_) : clientId(o_.clientId), quantity(o_.quantity), next(nullptr) {}

        ClientId clientId;
        Quantity quantity;
        RestingOrder *next;
    };

    using AlignedRestingOrder = CacheAligned<RestingOrder, 32>;

    static_assert(sizeof(AlignedRestingOrder) <= 64U, "Resting order does not fit in a cache line");
}