#pragma once

#include <cstddef>
#include <cstdlib>
#include <type_traits>

namespace exchange::utils
{
    template <typename T, std::size_t N>
    class SlabAllocator
    {
        static_assert(sizeof(T) >= sizeof(void *), "Cannot use SlabAllocator for small types");
        // TODO: SlabMap is complaining over this. For now I have cache-aligned Slot itself.
        // static_assert(alignof(T) >= 64 || sizeof(T) == alignof(T), "Allocation may straddle cache lines");

    public:
        using value_type = T;

        template <typename U>
        struct rebind
        {
            using other = SlabAllocator<U, N>;
        };

        SlabAllocator() : buffer(new Slot[N]), head(nullptr)
        {
            for (std::size_t i = N; i--;)
            {
                auto newHead = buffer + i;
                newHead->next = head;
                head = newHead;
            }
        }

        ~SlabAllocator()
        {
            delete[] buffer;
        }

        // Returns raw memory
        [[nodiscard]] T *allocate(const std::size_t n) noexcept
        {
            if (n > 1) [[unlikely]]
            {
                return static_cast<T *>(std::malloc(sizeof(T) * n));
            }
            if (!head) [[unlikely]]
            {
                return nullptr;
            }
            auto ptr = &head->storage;
            head = head->next;
            return ptr;
        }

        // This memory should have been received through allocate
        void deallocate(T *ptr, const std::size_t n) noexcept
        {
            if (n > 1) [[unlikely]]
            {
                std::free(static_cast<void *>(ptr));
                return;
            }
            auto newHead = reinterpret_cast<Slot *>(ptr);
            newHead->next = head;
            head = newHead;
        }

        SlabAllocator(const SlabAllocator &other) = delete;
        SlabAllocator(SlabAllocator &&other) = delete;

        SlabAllocator &operator=(const SlabAllocator &other) = delete;
        SlabAllocator &operator=(SlabAllocator &&other) = delete;

    private:
        union alignas(64) Slot
        {
            Slot *next;
            T storage;

            Slot() {}
            ~Slot() {}
        };
        Slot *buffer;
        Slot *head;
    };
}