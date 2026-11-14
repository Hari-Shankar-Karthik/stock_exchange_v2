#include <benchmark/benchmark.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include "book/resting_order.hpp"
#include "utils/slab_allocator.hpp"

// The allocated type is the real one the order book allocates (a 64-byte, cache-line aligned node).
// std::allocator is the baseline: for an over-aligned type it goes through the aligned operator new.
using Node = exchange::book::AlignedRestingOrder;
using SlabAlloc = exchange::utils::SlabAllocator<Node, 1024>;
using StdAlloc = std::allocator<Node>;

// Allocate one node and free it straight away: the best case, the same hot slot every time.
template <typename Alloc>
static void BM_Alloc_FreePair(benchmark::State &state)
{
    Alloc alloc;
    for (auto _ : state)
    {
        Node *p = alloc.allocate(1);
        benchmark::DoNotOptimize(p); // stops the compiler from removing the allocate/deallocate pair
        alloc.deallocate(p, 1);
    }
    state.SetItemsProcessed(static_cast<std::int64_t>(state.iterations()));
}
BENCHMARK(BM_Alloc_FreePair<SlabAlloc>)->Name("Alloc/FreePair/Slab");
BENCHMARK(BM_Alloc_FreePair<StdAlloc>)->Name("Alloc/FreePair/Std");

// Allocate Arg nodes, then free them all in the same order. Closer to a book filling up and draining:
// the slab walks its free list, the system allocator walks its bins.
template <typename Alloc>
static void BM_Alloc_FreeBatch(benchmark::State &state)
{
    const auto batch = static_cast<std::size_t>(state.range(0));
    Alloc alloc;
    std::vector<Node *> ptrs(batch);
    for (auto _ : state)
    {
        for (Node *&p : ptrs)
        {
            p = alloc.allocate(1);
        }
        benchmark::DoNotOptimize(ptrs.data());
        for (Node *p : ptrs)
        {
            alloc.deallocate(p, 1);
        }
    }
    state.SetItemsProcessed(static_cast<std::int64_t>(state.iterations() * batch));
}
BENCHMARK(BM_Alloc_FreeBatch<SlabAlloc>)->Name("Alloc/FreeBatch/Slab")->Arg(16)->Arg(512);
BENCHMARK(BM_Alloc_FreeBatch<StdAlloc>)->Name("Alloc/FreeBatch/Std")->Arg(16)->Arg(512);
