#include <gtest/gtest.h>

#include <thread>

#include "utils/spsc_queue.hpp"

using exchange::utils::SPSCQueue;

TEST(SPSCQueue, PopOnEmptyReturnsFalse)
{
    SPSCQueue<int, 4> q;
    int out = -1;
    EXPECT_FALSE(q.pop(out));
    EXPECT_EQ(out, -1); // untouched on failure
}

TEST(SPSCQueue, PushOnFullReturnsFalse)
{
    SPSCQueue<int, 4> q;
    for (int i = 0; i < 4; ++i)
        ASSERT_TRUE(q.push(i));
    EXPECT_FALSE(q.push(99));
}

TEST(SPSCQueue, PreservesFifoOrder)
{
    SPSCQueue<int, 4> q;
    for (int i = 0; i < 3; ++i)
        ASSERT_TRUE(q.push(i));
    for (int i = 0; i < 3; ++i)
    {
        int out = -1;
        ASSERT_TRUE(q.pop(out));
        EXPECT_EQ(out, i);
    }
}

TEST(SPSCQueue, WrapsAroundCorrectly)
{
    SPSCQueue<int, 4> q;
    int out;
    for (int round = 0; round < 10; ++round) // many more pushes than N
    {
        ASSERT_TRUE(q.push(round));
        ASSERT_TRUE(q.pop(out));
        EXPECT_EQ(out, round);
    }
}

TEST(SPSCQueue, TwoThreadsDeliverEverythingInOrder)
{
    constexpr int COUNT = 1'000'000;
    SPSCQueue<int, 1024> q;

    std::thread producer([&]
                         { for (int i = 0; i < COUNT; ++i) while (!q.push(i)) {} });

    int expected = 0;
    int out;
    while (expected < COUNT)
    {
        if (q.pop(out))
        {
            ASSERT_EQ(out, expected);
            ++expected;
        }
    }
    producer.join();
}
