// Benchmarks for LightThread context switching performance

#include <benchmark/benchmark.h>
#include "LightThread.h"
#include "Pointer.h"
#include <boost/container/pmr/unsynchronized_pool_resource.hpp>
#include <boost/container/pmr/monotonic_buffer_resource.hpp>

using namespace hbthreads;

namespace {
}  // namespace

// Thread that never finishes - for benchmarking pure context switch
class InfiniteYieldThread : public LightThread {
public:
    int yield_count = 0;

    void run() override {
        while (true) {
            yield_count++;
            wait();
        }
    }
};

// Benchmark raw context switch overhead
static void BM_ContextSwitch(benchmark::State& state) {
    boost::container::pmr::monotonic_buffer_resource mono(1024 * 1024);
    boost::container::pmr::unsynchronized_pool_resource pool(&mono);
    storage = &pool;

    Pointer<InfiniteYieldThread> thread(new InfiniteYieldThread());
    thread->start(64 * 1024);

    Event event{EventType::SocketRead, {0}};

    for (auto _ : state) {
        thread->resume(&event);
    }

    benchmark::DoNotOptimize(thread->yield_count);
}
BENCHMARK(BM_ContextSwitch);

// Benchmark context switch with varying stack sizes
static void BM_ContextSwitch_StackSize(benchmark::State& state) {
    boost::container::pmr::monotonic_buffer_resource mono(1024 * 1024);
    boost::container::pmr::unsynchronized_pool_resource pool(&mono);
    storage = &pool;

    const size_t stack_size = state.range(0) * 1024;

    Pointer<InfiniteYieldThread> thread(new InfiniteYieldThread());
    thread->start(stack_size);

    Event event{EventType::SocketRead, {0}};

    for (auto _ : state) {
        thread->resume(&event);
    }

    benchmark::DoNotOptimize(thread->yield_count);
    state.SetLabel(std::to_string(state.range(0)) + "KB stack");
}
BENCHMARK(BM_ContextSwitch_StackSize)
    ->Arg(16)   // 16KB (minimum safe with guard page)
    ->Arg(32)   // 32KB
    ->Arg(64)   // 64KB
    ->Arg(128)  // 128KB
    ->Arg(256); // 256KB

// Benchmark multiple concurrent threads context switching
static void BM_MultipleThreads(benchmark::State& state) {
    boost::container::pmr::monotonic_buffer_resource mono(64 * 1024 * 1024);  // 64MB for many threads
    boost::container::pmr::unsynchronized_pool_resource pool(&mono);
    storage = &pool;

    const int num_threads = state.range(0);

    std::vector<Pointer<InfiniteYieldThread>> threads;
    threads.reserve(num_threads);

    // Create threads once before benchmarking
    for (int i = 0; i < num_threads; i++) {
        threads.emplace_back(new InfiniteYieldThread());
        threads.back()->start(32 * 1024);
    }

    Event event{EventType::SocketRead, {0}};

    for (auto _ : state) {
        // Round-robin resume all threads once
        for (auto& thread : threads) {
            thread->resume(&event);
        }
    }

    int64_t total_yields = 0;
    for (auto& thread : threads) {
        total_yields += thread->yield_count;
    }
    benchmark::DoNotOptimize(total_yields);
    state.SetItemsProcessed(state.iterations() * num_threads);
}
BENCHMARK(BM_MultipleThreads)
    ->Arg(1)
    ->Arg(10)
    ->Arg(100)
    ->Arg(1000);
