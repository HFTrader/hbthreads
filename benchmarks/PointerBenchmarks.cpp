// Benchmarks for Pointer<T> intrusive smart pointer operations

#include <benchmark/benchmark.h>
#include "Pointer.h"
#include <boost/container/pmr/unsynchronized_pool_resource.hpp>
#include <memory>
#include <vector>

using namespace hbthreads;

namespace {

// Simple test object for pointer benchmarks
class TestObject : public Object {
public:
    int value;
    explicit TestObject(int v = 0) : value(v) {}
};

}  // namespace

// Benchmark Pointer<T> creation/destruction
static void BM_PointerCreateDestroy(benchmark::State& state) {
    boost::container::pmr::unsynchronized_pool_resource pool;
    storage = &pool;

    for (auto _ : state) {
        Pointer<TestObject> ptr(new TestObject(42));
        benchmark::DoNotOptimize(ptr.get());
    }
}
BENCHMARK(BM_PointerCreateDestroy);

// Benchmark std::shared_ptr for comparison
static void BM_SharedPtrCreateDestroy(benchmark::State& state) {
    for (auto _ : state) {
        auto ptr = std::make_shared<int>(42);
        benchmark::DoNotOptimize(ptr.get());
    }
}
BENCHMARK(BM_SharedPtrCreateDestroy);

// Benchmark Pointer<T> copy (reference count increment)
static void BM_PointerCopy(benchmark::State& state) {
    boost::container::pmr::unsynchronized_pool_resource pool;
    storage = &pool;

    Pointer<TestObject> original(new TestObject(42));

    for (auto _ : state) {
        Pointer<TestObject> copy = original;
        benchmark::DoNotOptimize(copy.get());
    }
}
BENCHMARK(BM_PointerCopy);

// Benchmark std::shared_ptr copy for comparison
static void BM_SharedPtrCopy(benchmark::State& state) {
    auto original = std::make_shared<int>(42);

    for (auto _ : state) {
        auto copy = original;
        benchmark::DoNotOptimize(copy.get());
    }
}
BENCHMARK(BM_SharedPtrCopy);

// Benchmark Pointer<T> dereference
static void BM_PointerDeref(benchmark::State& state) {
    boost::container::pmr::unsynchronized_pool_resource pool;
    storage = &pool;

    Pointer<TestObject> ptr(new TestObject(42));
    int sum = 0;

    for (auto _ : state) {
        sum += ptr->value;
        benchmark::DoNotOptimize(sum);
    }

    benchmark::DoNotOptimize(sum);
}
BENCHMARK(BM_PointerDeref);

// Benchmark pool allocation throughput
static void BM_PoolAllocation(benchmark::State& state) {
    boost::container::pmr::unsynchronized_pool_resource pool;
    storage = &pool;

    const int batch_size = state.range(0);

    for (auto _ : state) {
        std::vector<Pointer<TestObject>> objects;
        objects.reserve(batch_size);

        for (int i = 0; i < batch_size; i++) {
            objects.emplace_back(new TestObject(i));
        }

        benchmark::DoNotOptimize(objects.data());
        // Objects destroyed here
    }

    state.SetItemsProcessed(state.iterations() * batch_size);
}
BENCHMARK(BM_PoolAllocation)
    ->Arg(10)
    ->Arg(100)
    ->Arg(1000)
    ->Arg(10000);

// Benchmark std::shared_ptr allocation for comparison
static void BM_SharedPtrAllocation(benchmark::State& state) {
    const int batch_size = state.range(0);

    for (auto _ : state) {
        std::vector<std::shared_ptr<int>> objects;
        objects.reserve(batch_size);

        for (int i = 0; i < batch_size; i++) {
            objects.emplace_back(std::make_shared<int>(i));
        }

        benchmark::DoNotOptimize(objects.data());
    }

    state.SetItemsProcessed(state.iterations() * batch_size);
}
BENCHMARK(BM_SharedPtrAllocation)
    ->Arg(10)
    ->Arg(100)
    ->Arg(1000)
    ->Arg(10000);

// Benchmark reference count churn
static void BM_RefCountChurn(benchmark::State& state) {
    boost::container::pmr::unsynchronized_pool_resource pool;
    storage = &pool;

    Pointer<TestObject> ptr(new TestObject(42));
    const int churn_count = 100;

    for (auto _ : state) {
        for (int i = 0; i < churn_count; i++) {
            Pointer<TestObject> temp = ptr;
            benchmark::DoNotOptimize(temp.get());
        }
    }

    state.SetItemsProcessed(state.iterations() * churn_count);
}
BENCHMARK(BM_RefCountChurn);
