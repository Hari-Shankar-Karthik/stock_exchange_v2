#pragma once

#include <cstddef>

namespace exchange::core
{
    constexpr std::size_t INCOMING_BUFFER_SIZE = 1 << 10; // 1 << 20
    constexpr std::size_t EVENT_BUFFER_SIZE = 1 << 10;    // 1 << 20

    constexpr std::size_t MAX_LEVELS = 1 << 10;         // 10'000
    constexpr std::size_t MAX_RESTING_ORDERS = 1 << 15; // 100'000'000

    constexpr std::size_t MAX_CLIENTS = 8; // TODO: Review this number
    // constexpr std::size_t MAX_QUEUED_EVENTS = 16; // TODO: Review this number

    constexpr std::uint64_t PAGE_SIZE_BYTES = 4096;
    static_assert(!(PAGE_SIZE_BYTES & (PAGE_SIZE_BYTES - 1)), "Page size should be a power of 2");

    constexpr std::uint64_t REPLAY_LOG_PAGES = 2;
}