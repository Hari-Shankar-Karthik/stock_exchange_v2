#pragma once

#include <iostream>
#include <cstdint>

namespace exchange::core
{
    using ClientId = std::uint64_t;

    using Price = std::int64_t;

    using Quantity = std::uint32_t;

    using SequenceNumber = std::uint64_t;

    using ExecutionId = std::uint64_t;

    enum class Side : std::uint8_t
    {
        BUY,
        SELL,
    };

    enum class OrderType : std::uint8_t
    {
        LIMIT,
        MARKET,
        IOC,
    };

    inline std::ostream &operator<<(std::ostream &os, const Side &s) noexcept
    {
        if (s == Side::BUY)
        {
            os << "BUY";
        }
        else if (s == Side::SELL)
        {
            os << "SELL";
        }
        return os;
    }

    inline std::ostream &operator<<(std::ostream &os, const OrderType &t) noexcept
    {
        if (t == OrderType::LIMIT)
        {
            os << "LIMIT";
        }
        else if (t == OrderType::MARKET)
        {
            os << "MARKET";
        }
        else if (t == OrderType::IOC)
        {
            os << "IOC";
        }
        return os;
    }
}