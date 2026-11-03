#pragma once

#include <iostream>
#include <cstdint>

namespace exchange::core
{
    using ClientId = std::uint64_t;

    using Price = std::int64_t;

    using Quantity = std::uint32_t;

    enum class Side : std::uint8_t
    {
        BUY,
        SELL,
    };

    using SequenceNumber = std::uint64_t;

    using ExecutionId = std::uint64_t;

    inline std::ostream &operator<<(std::ostream &os, const Side &s) noexcept
    {
        if (s == Side::BUY)
        {
            os << "BUY";
        }
        else
        {
            os << "SELL";
        }
        return os;
    }
}