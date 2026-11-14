#include <gtest/gtest.h>

#include "book/order_book.hpp"
#include "core/order.hpp"
#include "core/trade_event.hpp"

using exchange::book::OrderBook;
using exchange::core::ClientId;
using exchange::core::Order;
using exchange::core::OrderType;
using exchange::core::Price;
using exchange::core::Quantity;
using exchange::core::SequenceNumber;
using exchange::core::Side;
using exchange::core::TradeEvent;
using exchange::helper::TradeQueue;

struct OrderBookTest : public ::testing::Test
{
    OrderBookTest()
        : book(tradeEvents, marketData)
    {
    }

    void processLimitOrder(Side side, SequenceNumber seqNum, ClientId clientId, Price price, Quantity quantity)
    {
        book.process(Order{OrderType::LIMIT, side, seqNum, clientId, price, quantity});
    }

    TradeQueue tradeEvents, marketData;
    OrderBook book;
};

TEST_F(OrderBookTest, CrossingSellFillsRestingBuy)
{
    processLimitOrder(Side::BUY, 1, 1, 100, 10);
    processLimitOrder(Side::SELL, 2, 2, 99, 10);

    TradeEvent e;
    ASSERT_TRUE(tradeEvents.pop(e));
    EXPECT_EQ(e.buyerId, 1);
    EXPECT_EQ(e.sellerId, 2);
    EXPECT_EQ(e.price, 100);
    EXPECT_EQ(e.quantity, 10);
}

TEST_F(OrderBookTest, CrossingBuyFillsRestingSell)
{
    processLimitOrder(Side::SELL, 1, 1, 100, 10);
    processLimitOrder(Side::BUY, 2, 2, 101, 10);

    TradeEvent e;
    ASSERT_TRUE(tradeEvents.pop(e));
    EXPECT_EQ(e.buyerId, 2);
    EXPECT_EQ(e.sellerId, 1);
    EXPECT_EQ(e.price, 100);
    EXPECT_EQ(e.quantity, 10);
}

TEST_F(OrderBookTest, CrossingSellFillsMultipleRestingBuys)
{
    processLimitOrder(Side::BUY, 1, 1, 100, 10);
    processLimitOrder(Side::BUY, 2, 2, 100, 20);
    processLimitOrder(Side::BUY, 3, 3, 100, 30);
    processLimitOrder(Side::SELL, 4, 4, 99, 60);

    TradeEvent e;

    ASSERT_TRUE(tradeEvents.pop(e));
    EXPECT_EQ(e.buyerId, 1);
    EXPECT_EQ(e.sellerId, 4);
    EXPECT_EQ(e.price, 100);
    EXPECT_EQ(e.quantity, 10);

    ASSERT_TRUE(tradeEvents.pop(e));
    EXPECT_EQ(e.buyerId, 2);
    EXPECT_EQ(e.sellerId, 4);
    EXPECT_EQ(e.price, 100);
    EXPECT_EQ(e.quantity, 20);

    ASSERT_TRUE(tradeEvents.pop(e));
    EXPECT_EQ(e.buyerId, 3);
    EXPECT_EQ(e.sellerId, 4);
    EXPECT_EQ(e.price, 100);
    EXPECT_EQ(e.quantity, 30);
}

TEST_F(OrderBookTest, CrossingBuyFillsMultipleRestingSells)
{
    processLimitOrder(Side::SELL, 1, 1, 100, 10);
    processLimitOrder(Side::SELL, 2, 2, 100, 20);
    processLimitOrder(Side::SELL, 3, 3, 100, 30);
    processLimitOrder(Side::BUY, 4, 4, 101, 60);

    TradeEvent e;

    ASSERT_TRUE(tradeEvents.pop(e));
    EXPECT_EQ(e.buyerId, 4);
    EXPECT_EQ(e.sellerId, 1);
    EXPECT_EQ(e.price, 100);
    EXPECT_EQ(e.quantity, 10);

    ASSERT_TRUE(tradeEvents.pop(e));
    EXPECT_EQ(e.buyerId, 4);
    EXPECT_EQ(e.sellerId, 2);
    EXPECT_EQ(e.price, 100);
    EXPECT_EQ(e.quantity, 20);

    ASSERT_TRUE(tradeEvents.pop(e));
    EXPECT_EQ(e.buyerId, 4);
    EXPECT_EQ(e.sellerId, 3);
    EXPECT_EQ(e.price, 100);
    EXPECT_EQ(e.quantity, 30);
}

TEST_F(OrderBookTest, SpilloverSellRestsOnBook)
{
    processLimitOrder(Side::BUY, 1, 1, 100, 10);
    processLimitOrder(Side::SELL, 2, 2, 99, 20);
    processLimitOrder(Side::BUY, 3, 3, 100, 10);

    TradeEvent e;

    ASSERT_TRUE(tradeEvents.pop(e));
    EXPECT_EQ(e.buyerId, 1);
    EXPECT_EQ(e.sellerId, 2);
    EXPECT_EQ(e.price, 100);
    EXPECT_EQ(e.quantity, 10);

    ASSERT_TRUE(tradeEvents.pop(e));
    EXPECT_EQ(e.buyerId, 3);
    EXPECT_EQ(e.sellerId, 2);
    EXPECT_EQ(e.price, 99);
    EXPECT_EQ(e.quantity, 10);
}

TEST_F(OrderBookTest, SpilloverBuyRestsOnBook)
{
    processLimitOrder(Side::SELL, 1, 1, 100, 10);
    processLimitOrder(Side::BUY, 2, 2, 101, 20);
    processLimitOrder(Side::SELL, 3, 3, 100, 10);

    TradeEvent e;

    ASSERT_TRUE(tradeEvents.pop(e));
    EXPECT_EQ(e.buyerId, 2);
    EXPECT_EQ(e.sellerId, 1);
    EXPECT_EQ(e.price, 100);
    EXPECT_EQ(e.quantity, 10);

    ASSERT_TRUE(tradeEvents.pop(e));
    EXPECT_EQ(e.buyerId, 2);
    EXPECT_EQ(e.sellerId, 3);
    EXPECT_EQ(e.price, 101);
    EXPECT_EQ(e.quantity, 10);
}

TEST_F(OrderBookTest, PricePriorityAmongSells)
{
    processLimitOrder(Side::SELL, 1, 1, 101, 10);
    processLimitOrder(Side::SELL, 2, 2, 100, 10);
    processLimitOrder(Side::BUY, 3, 3, 101, 10);

    TradeEvent e;
    ASSERT_TRUE(tradeEvents.pop(e));
    EXPECT_EQ(e.buyerId, 3);
    EXPECT_EQ(e.sellerId, 2);
    EXPECT_EQ(e.price, 100);
    EXPECT_EQ(e.quantity, 10);
}

TEST_F(OrderBookTest, PricePriorityAmongBuys)
{
    processLimitOrder(Side::BUY, 1, 1, 99, 10);
    processLimitOrder(Side::BUY, 2, 2, 100, 10);
    processLimitOrder(Side::SELL, 3, 3, 99, 10);

    TradeEvent e;
    ASSERT_TRUE(tradeEvents.pop(e));
    EXPECT_EQ(e.buyerId, 2);
    EXPECT_EQ(e.sellerId, 3);
    EXPECT_EQ(e.price, 100);
    EXPECT_EQ(e.quantity, 10);
}