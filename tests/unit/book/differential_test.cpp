#include <cstddef>
#include <gtest/gtest.h>

#include "book/order_book.hpp"
#include "core/trade_event.hpp"
#include "helper/types.hpp"
#include "support/reference_book.hpp"
#include "support/order_generator.hpp"

using exchange::book::OrderBook;
using exchange::core::TradeEvent;
using exchange::helper::TradeQueue;
using exchange::test::OrderGenerator;
using exchange::test::ReferenceBook;

struct OrderBookDifferentialTest : public ::testing::Test
{
    OrderBookDifferentialTest() : g(69), mybook(tradeEvents, marketData)
    {
    }

    constexpr static std::uint32_t NUM_ORDERS = 8'192;
    OrderGenerator g;
    TradeQueue tradeEvents, marketData;
    OrderBook mybook;
    ReferenceBook refBook;
};

TEST_F(OrderBookDifferentialTest, RandomSimulation)
{
    TradeEvent e;
    for (std::uint32_t i = 0; i < NUM_ORDERS; ++i)
    {
        const auto o = g.next();
        mybook.process(o);
        const auto trades = refBook.process(o);
        for (const auto &trade : trades)
        {
            ASSERT_TRUE(tradeEvents.pop(e));
            EXPECT_EQ(e, trade);
        }
    }
}