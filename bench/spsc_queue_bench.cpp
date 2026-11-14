#include <benchmark/benchmark.h>

#include <cstdint>

#include "core/constants.hpp"
#include "core/order.hpp"
#include "helper/types.hpp"

using exchange::core::AlignedOrder;
using exchange::core::INCOMING_BUFFER_SIZE;
using exchange::helper::OrderQueue;

// One push immediately followed by one pop, on one thread: the uncontended cost of the queue code itself.
static void BM_SPSCQueue_PushPop_SameThread(benchmark::State &state)
{
    OrderQueue q;
    AlignedOrder in{};
    AlignedOrder out{};
    for (auto _ : state)
    {
        benchmark::DoNotOptimize(q.push(in));
        benchmark::DoNotOptimize(q.pop(out));
    }
    benchmark::DoNotOptimize(out);
    state.SetItemsProcessed(static_cast<std::int64_t>(state.iterations()));
}
BENCHMARK(BM_SPSCQueue_PushPop_SameThread);

// Push Arg items, then pop Arg items, on one thread. Larger batches walk further around the ring
// buffer (more cache lines touched) and exercise the cached head/tail refresh path.
static void BM_SPSCQueue_Burst_SameThread(benchmark::State &state)
{
    const auto batch = static_cast<std::size_t>(state.range(0));
    if (batch > INCOMING_BUFFER_SIZE)
    {
        state.SkipWithError("batch exceeds the queue capacity");
        return;
    }
    OrderQueue q;
    AlignedOrder in{};
    AlignedOrder out{};
    for (auto _ : state)
    {
        for (std::size_t i = 0; i < batch; ++i)
        {
            q.push(in);
        }
        for (std::size_t i = 0; i < batch; ++i)
        {
            q.pop(out);
        }
        benchmark::DoNotOptimize(out);
    }
    state.SetItemsProcessed(static_cast<std::int64_t>(state.iterations() * batch));
}
BENCHMARK(BM_SPSCQueue_Burst_SameThread)->Arg(1)->Arg(8)->Arg(64)->Arg(512)->Arg(1024);

// Two threads: thread 0 produces, thread 1 consumes, both spinning. Measures sustained cross-core throughput.
// NEEDS TWO CPUs: run with `taskset -c 2,3` (different physical cores). Pinned to ONE cpu, the spinning
// threads wait out each other's time slices and the benchmark crawls.
static void BM_SPSCQueue_CrossThread_Throughput(benchmark::State &state)
{
    static OrderQueue *queue = nullptr;
    if (state.thread_index() == 0)
    {
        queue = new OrderQueue();
    }

    // The framework synchronises all threads at the start and the end of the timed loop, so the queue
    // exists while any thread is inside it, and is not deleted while another thread is still using it.
    AlignedOrder item{};
    if (state.thread_index() == 0)
    {
        for (auto _ : state)
        {
            while (!queue->push(item))
            {
            }
        }
        state.SetItemsProcessed(static_cast<std::int64_t>(state.iterations())); // count items once, not per thread
    }
    else
    {
        for (auto _ : state)
        {
            while (!queue->pop(item))
            {
            }
        }
    }
    benchmark::DoNotOptimize(item);

    if (state.thread_index() == 0)
    {
        delete queue;
        queue = nullptr;
    }
}
BENCHMARK(BM_SPSCQueue_CrossThread_Throughput)->Threads(2)->UseRealTime();

// Ping-pong: thread 0 sends an item, thread 1 echoes it back, thread 0 waits for the echo.
// One iteration is a ROUND TRIP, so one-way cross-core latency is about half the reported time.
// NEEDS TWO CPUs, see above.
static void BM_SPSCQueue_CrossThread_PingPong(benchmark::State &state)
{
    static OrderQueue *ping = nullptr; // thread 0 -> thread 1
    static OrderQueue *pong = nullptr; // thread 1 -> thread 0
    if (state.thread_index() == 0)
    {
        ping = new OrderQueue();
        pong = new OrderQueue();
    }

    AlignedOrder item{};
    if (state.thread_index() == 0)
    {
        for (auto _ : state)
        {
            while (!ping->push(item))
            {
            }
            while (!pong->pop(item))
            {
            }
        }
    }
    else
    {
        for (auto _ : state)
        {
            while (!ping->pop(item))
            {
            }
            while (!pong->push(item))
            {
            }
        }
    }
    benchmark::DoNotOptimize(item);

    if (state.thread_index() == 0)
    {
        delete ping;
        delete pong;
        ping = pong = nullptr;
    }
}
BENCHMARK(BM_SPSCQueue_CrossThread_PingPong)->Threads(2)->UseRealTime();
