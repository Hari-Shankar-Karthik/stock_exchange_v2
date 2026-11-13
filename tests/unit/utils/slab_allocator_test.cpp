#include <gtest/gtest.h>

#include <array>
#include <set>

#include "utils/slab_allocator.hpp"

using exchange::utils::SlabAllocator;

using Blob = std::array<std::byte, 64>;

TEST(SlabAllocator, HandsOutDistinctSlotsUntilExhausted)
{
    SlabAllocator<Blob, 4> a;
    std::set<Blob *> seen;
    for (int i = 0; i < 4; ++i)
    {
        Blob *p = a.allocate(1);
        ASSERT_NE(p, nullptr);
        EXPECT_FALSE(seen.contains(p)) << "slot handed out twice";
        seen.insert(p);
    }
    EXPECT_EQ(a.allocate(1), nullptr); // exhausted
}

TEST(SlabAllocator, ReusesFreedSlot)
{
    SlabAllocator<Blob, 1> a;
    Blob *first = a.allocate(1);
    ASSERT_NE(first, nullptr);
    a.deallocate(first, 1);
    EXPECT_EQ(a.allocate(1), first);
}

TEST(SlabAllocator, LargeAllocation)
{
    SlabAllocator<Blob, 1'024> a;
    Blob *arr = a.allocate(128);
    ASSERT_NE(arr, nullptr);
    a.deallocate(arr, 128);
}