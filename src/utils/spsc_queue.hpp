#pragma once

#include <atomic>
#include <cstdint>
#include <optional>
#include <type_traits>

namespace exchange::utils
{
    // Typically for use-cases where T fits in a single cache line.
    template <typename T, std::size_t N>
    class SPSCQueue
    {
        static_assert(std::is_default_constructible_v<T>);
        static_assert(std::is_trivially_copyable_v<T>);
        static_assert(N > 0 && (N & (N - 1)) == 0, "N should be a positive power of 2");

    public:
        // called by the consumer
        bool pop(T &ele) noexcept
        {
            const auto currentHead = head.load(std::memory_order_relaxed);
            // check emptiness
            if (currentHead == cachedTail)
            {
                cachedTail = tail.load(std::memory_order_acquire);
                if (currentHead == cachedTail)
                {
                    return false;
                }
            }
            ele = buffer[currentHead & MASK];
            head.store(1 + currentHead, std::memory_order_release);
            return true;
        }

        // called by the producer
        bool push(T ele) noexcept
        {
            const auto currentTail = tail.load(std::memory_order_relaxed);
            // check fullness
            if (currentTail - cachedHead == N)
            {
                cachedHead = head.load(std::memory_order_acquire);
                if (currentTail - cachedHead == N)
                {
                    return false;
                }
            }
            buffer[currentTail & MASK] = ele;
            tail.store(1 + currentTail, std::memory_order_release);
            return true;
        }

    private:
        alignas(64) std::atomic<std::uint64_t> head{0};
        alignas(64) std::atomic<std::uint64_t> tail{0};
        alignas(64) std::uint64_t cachedHead{0};
        alignas(64) std::uint64_t cachedTail{0};
        alignas(64) T buffer[N]{};
        constexpr static std::size_t MASK = N - 1;
    };
}