#pragma once

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
}