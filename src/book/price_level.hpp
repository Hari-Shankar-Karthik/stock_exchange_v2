#pragma once

#include "book/resting_order.hpp"

namespace exchange::book
{
    struct PriceLevel
    {
        RestingOrder *head{nullptr};
        RestingOrder *tail{nullptr};

        void insert(RestingOrder *ptr) noexcept
        {
            if (!head)
            {
                head = tail = ptr;
            }
            else
            {
                tail->next = ptr;
                tail = ptr;
            }
        }
    };
}
