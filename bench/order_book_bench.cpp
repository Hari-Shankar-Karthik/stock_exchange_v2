#include <benchmark/benchmark.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <vector>

#include "book/order_book.hpp"
#include "support/order_generator.hpp"
#include "support/reference_book.hpp"

using exchange::book::OrderBook;
using exchange::core::ClientId;
using exchange::core::MAX_LEVELS;
using exchange::core::MAX_RESTING_ORDERS;
using exchange::core::Order;
using exchange::core::OrderType;
using exchange::core::Price;
using exchange::core::Quantity;
using exchange::core::Side;
using exchange::core::TradeEvent;
using exchange::helper::TradeQueue;

namespace
{
    Order makeOrder(OrderType type, Side side, Price price, Quantity quantity, ClientId client = 1)
    {
        return Order{type, side, 0, client, price, quantity};
    }

    void drain(TradeQueue &q)
    {
        TradeEvent e;
        while (q.pop(e))
        {
        }
    }

    // An OrderBook plus the two queues it publishes to. A book never forgets resting orders (there is
    // no cancel yet), so benchmarks that insert a lot start from a fresh book every iteration. Building
    // one allocates and links the slabs, which is not what we want to measure, hence resetUntimed().
    class BookUnderTest
    {
    public:
        BookUnderTest() : book(std::make_unique<OrderBook>(tradeEvents, marketData)) {}

        OrderBook &operator*() { return *book; }

        void drainQueues()
        {
            drain(tradeEvents);
            drain(marketData);
        }

        void resetUntimed(benchmark::State &state)
        {
            state.PauseTiming(); // costs ~1 us, so only use it around work that takes much longer than that
            book.reset();
            drainQueues();
            book = std::make_unique<OrderBook>(tradeEvents, marketData);
            state.ResumeTiming();
        }

    private:
        TradeQueue tradeEvents;
        TradeQueue marketData;
        std::unique_ptr<OrderBook> book;
    };

    // Built once, outside any timed region.
    std::vector<Order> makeStream(std::uint64_t seed, std::size_t count)
    {
        exchange::test::ReferenceBook reference; // only used to check the stream fits in the engine's slabs
        exchange::test::OrderGenerator generator{seed};
        std::vector<Order> orders;
        orders.reserve(count);
        std::size_t peakResting = 0;
        for (std::size_t i = 0; i < count; ++i)
        {
            const Order o = generator.next();
            reference.process(o);
            peakResting = std::max(peakResting, reference.restingCount());
            orders.push_back(o);
        }
        if (peakResting >= MAX_RESTING_ORDERS)
        {
            std::cerr << "benchmark stream rests up to " << peakResting << " orders, but the engine's slab holds "
                      << MAX_RESTING_ORDERS << "; raise MAX_RESTING_ORDERS or shorten the stream\n";
            std::abort();
        }
        return orders;
    }
}

// Cost of building an empty book (slab initialisation). Explains the fixed overhead of the other benchmarks.
static void BM_OrderBook_Construct(benchmark::State &state)
{
    TradeQueue tradeEvents, marketData;
    for (auto _ : state)
    {
        OrderBook book{tradeEvents, marketData};
        benchmark::DoNotOptimize(&book);
    }
}
BENCHMARK(BM_OrderBook_Construct);

// Throughput on a realistic random stream (mix of LIMIT/MARKET/IOC around a narrow price band).
// The seed is fixed ON PURPOSE: every run and every version of the code must see the same input.
// Note: random flow with no cancels grows a deep book, so this is "deep book" performance.
static void BM_OrderBook_RandomStream(benchmark::State &state)
{
    static const std::vector<Order> orders = makeStream(42, 100'000);
    BookUnderTest fixture;

    for (auto _ : state)
    {
        fixture.resetUntimed(state);
        OrderBook &book = *fixture;
        std::size_t n = 0;
        for (const Order &o : orders)
        {
            book.process(o);
            if ((++n & 255) == 0) // the 1024-slot queues would fill otherwise; a real consumer would be popping
            {
                fixture.drainQueues();
            }
        }
    }
    state.SetItemsProcessed(static_cast<std::int64_t>(state.iterations() * orders.size()));
}
BENCHMARK(BM_OrderBook_RandomStream)->Unit(benchmark::kMillisecond);

// Insert-only: buy orders below any ask, so nothing ever trades. Arg = number of distinct price levels
// the orders are spread over (1 = one long queue at a single price, 1000 = one order per level).
static void BM_OrderBook_RestNoCross(benchmark::State &state)
{
    const auto levels = static_cast<std::size_t>(state.range(0));
    constexpr std::size_t ORDERS_PER_ITERATION = 8192;
    static_assert(ORDERS_PER_ITERATION < MAX_RESTING_ORDERS);
    if (levels > MAX_LEVELS)
    {
        state.SkipWithError("more levels than the engine's slab holds");
        return;
    }

    BookUnderTest fixture;
    for (auto _ : state)
    {
        fixture.resetUntimed(state);
        OrderBook &book = *fixture;
        for (std::size_t i = 0; i < ORDERS_PER_ITERATION; ++i)
        {
            book.process(makeOrder(OrderType::LIMIT, Side::BUY, 1000 - static_cast<Price>(i % levels), 1));
        }
    }
    state.SetItemsProcessed(static_cast<std::int64_t>(state.iterations() * ORDERS_PER_ITERATION));
}
BENCHMARK(BM_OrderBook_RestNoCross)->Arg(1)->Arg(16)->Arg(256)->Arg(1000);

// Steady state at the best price. Arg = how many orders already queue at that price.
// Each iteration: an aggressive buy fully fills the oldest resting sell (a trade, one node freed), then a
// sell rests again at the back of the queue (one node allocated). Depth stays constant, so there is no reset.
static void BM_OrderBook_MatchBestLevel(benchmark::State &state)
{
    const auto depth = static_cast<std::size_t>(state.range(0));
    static_assert(MAX_RESTING_ORDERS > 10'000);

    BookUnderTest fixture;
    OrderBook &book = *fixture;
    for (std::size_t i = 0; i < depth; ++i)
    {
        book.process(makeOrder(OrderType::LIMIT, Side::SELL, 1001, 1));
    }

    std::size_t n = 0;
    for (auto _ : state)
    {
        book.process(makeOrder(OrderType::LIMIT, Side::BUY, 1001, 1));  // matches
        book.process(makeOrder(OrderType::LIMIT, Side::SELL, 1001, 1)); // rests
        if ((++n & 255) == 0)
        {
            fixture.drainQueues();
        }
    }
    state.SetItemsProcessed(static_cast<std::int64_t>(state.iterations() * 2));
}
BENCHMARK(BM_OrderBook_MatchBestLevel)->Arg(1)->Arg(100)->Arg(10'000);

// Sweep: Arg resting sells, one per price level, then one market buy that walks through all of them.
// Each iteration therefore includes rebuilding the levels (Arg inserts), so subtract
// BM_OrderBook_RestNoCross to estimate the sweep itself. Each level erased frees a map node.
static void BM_OrderBook_SweepLevels(benchmark::State &state)
{
    const auto levels = static_cast<std::size_t>(state.range(0));
    if (levels > MAX_LEVELS || levels > 1000) // 1 trade per level must fit in the 1024-slot trade queues
    {
        state.SkipWithError("too many levels");
        return;
    }

    BookUnderTest fixture;
    OrderBook &book = *fixture;
    for (auto _ : state)
    {
        for (std::size_t i = 0; i < levels; ++i)
        {
            book.process(makeOrder(OrderType::LIMIT, Side::SELL, 1001 + static_cast<Price>(i), 1));
        }
        book.process(makeOrder(OrderType::MARKET, Side::BUY, 0, static_cast<Quantity>(levels)));
        fixture.drainQueues(); // included in the time: it is a few ns per trade
    }
    state.SetItemsProcessed(static_cast<std::int64_t>(state.iterations() * (levels + 1)));
}
BENCHMARK(BM_OrderBook_SweepLevels)->Arg(1)->Arg(16)->Arg(128)->Arg(512);
