#pragma once

#include <atomic>
#include <cstddef>
#include <cstring>
#include <filesystem>
#include <fcntl.h>
#include <sys/mman.h>
#include <type_traits>
#include <unistd.h>

#include "config/config.hpp"
#include "core/constants.hpp"

namespace exchange::utils
{
    using config::REPLAY_LOG_DIR;
    using core::PAGE_SIZE_BYTES;

    template <typename T, std::uint64_t NUM_PAGES>
    class Log
    {
        static_assert(sizeof(T) == alignof(T), "Require well-aligned types");
        static_assert(sizeof(T) <= 64, "[SOFT] Require types which fit in a cache line");
        static_assert(std::is_trivially_default_constructible_v<T>);
        static_assert(std::is_trivially_copyable_v<T>);
        static_assert(!(NUM_PAGES & (NUM_PAGES - 1)), "Number of pages should be a power of 2");

    public:
        // TODO: Remove checks and make this noexcept
        Log(std::uint64_t seed)
        {
            // Pre-fault the pages in
            const auto ptr = mmap(NULL, NUM_PAGES * PAGE_SIZE_BYTES, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE, -1, 0);
            if (ptr == reinterpret_cast<void *>(-1))
            {
                perror("mmap");
            }
            basePtr = static_cast<std::byte *>(ptr);

            // There should not already be a log file with the same name
            const std::filesystem::path filePath = std::filesystem::path(REPLAY_LOG_DIR) / (std::to_string(seed) + ".log");
            // TODO: DEBUG ONLY: Remove O_EXCL
            fd = open(filePath.c_str(), O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
            if (fd < 0)
            {
                perror("open");
            }
        }

        ~Log()
        {
            // Drain the last incomplete page
            drainPage();
            if (munmap(static_cast<void *>(basePtr), NUM_PAGES * PAGE_SIZE_BYTES) < 0)
            {
                perror("munmap");
            }
            if (close(fd) < 0)
            {
                perror("close");
            }
        }

        Log(const Log &other) = delete;
        Log(Log &&other) = delete;

        Log &operator=(const Log &other) = delete;
        Log &operator=(Log &&other) = delete;

        void push(const T &ele) noexcept
        {
            *reinterpret_cast<T *>(basePtr + (writeOffset & ADDR_MASK)) = ele;
            writeOffset += sizeof(T);

            const auto currPageNumber = writeOffset & PAGE_MASK;
            if (pageNumber.load(std::memory_order_relaxed) != currPageNumber)
            {
                pageNumber.store(currPageNumber, std::memory_order_release);
            }
        }

        void drain() noexcept
        {
            while (true)
            {
                if (pageNumber.load(std::memory_order_acquire) != (drainOffset & PAGE_MASK))
                {
                    drainPage();
                }
            }
        }

    private:
        constexpr static std::uint64_t ADDR_MASK = NUM_PAGES * PAGE_SIZE_BYTES - 1;
        constexpr static std::uint64_t PAGE_MASK = (NUM_PAGES - 1) * PAGE_SIZE_BYTES;

        // Writer only
        std::uint64_t writeOffset = 0;

        // Shared between writer and drainer
        alignas(64) std::byte *basePtr = nullptr; // TODO: Make this a const pointer
        std::atomic<std::uint64_t> pageNumber = 0;

        // Drainer only
        alignas(64) int fd = -1;
        std::uint64_t drainOffset = 0;

        void drainPage() noexcept
        {
            const auto drainPageNumber = drainOffset & PAGE_MASK;
            std::cerr << "[DRAINER] Draining page starting at " << drainPageNumber << std::endl;
            pwrite(fd, basePtr + drainPageNumber, PAGE_SIZE_BYTES, drainOffset);
            // Zeroing out because this page could be the last (partial) page
            std::memset(basePtr + drainPageNumber, 0, PAGE_SIZE_BYTES);
            drainOffset += PAGE_SIZE_BYTES;
        }
    };
}
