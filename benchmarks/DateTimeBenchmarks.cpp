// Benchmarks for DateTime operations

#include <benchmark/benchmark.h>
#include "DateTime.h"
#include <chrono>

using namespace hbthreads;

// Benchmark DateTime::now()
static void BM_DateTimeNow(benchmark::State& state) {
    for (auto _ : state) {
        DateTime t = DateTime::now();
        benchmark::DoNotOptimize(t);
    }
}
BENCHMARK(BM_DateTimeNow);

// Benchmark std::chrono::steady_clock for comparison
static void BM_ChronoNow(benchmark::State& state) {
    for (auto _ : state) {
        auto t = std::chrono::steady_clock::now();
        benchmark::DoNotOptimize(t);
    }
}
BENCHMARK(BM_ChronoNow);

// Benchmark DateTime arithmetic
static void BM_DateTimeArithmetic(benchmark::State& state) {
    DateTime a = DateTime::now();
    DateTime b = DateTime::msecs(100);

    for (auto _ : state) {
        DateTime c = a + b;
        benchmark::DoNotOptimize(c);
    }
}
BENCHMARK(BM_DateTimeArithmetic);

// Benchmark DateTime comparison
static void BM_DateTimeCompare(benchmark::State& state) {
    DateTime a = DateTime::now();
    DateTime b = a + DateTime::usecs(1);

    for (auto _ : state) {
        bool result = a < b;
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_DateTimeCompare);

// Benchmark DateTime conversion to nanoseconds
static void BM_DateTimeToNsecs(benchmark::State& state) {
    DateTime t = DateTime::now();

    for (auto _ : state) {
        int64_t ns = t.nsecs();
        benchmark::DoNotOptimize(ns);
    }
}
BENCHMARK(BM_DateTimeToNsecs);

// Benchmark DateTime conversion to milliseconds
static void BM_DateTimeToMsecs(benchmark::State& state) {
    DateTime t = DateTime::now();

    for (auto _ : state) {
        int64_t ms = t.msecs();
        benchmark::DoNotOptimize(ms);
    }
}
BENCHMARK(BM_DateTimeToMsecs);

// Benchmark DateTime creation from various units
static void BM_DateTimeFromNsecs(benchmark::State& state) {
    for (auto _ : state) {
        DateTime t = DateTime::nsecs(1234567890);
        benchmark::DoNotOptimize(t);
    }
}
BENCHMARK(BM_DateTimeFromNsecs);

static void BM_DateTimeFromUsecs(benchmark::State& state) {
    for (auto _ : state) {
        DateTime t = DateTime::usecs(1234567);
        benchmark::DoNotOptimize(t);
    }
}
BENCHMARK(BM_DateTimeFromUsecs);

static void BM_DateTimeFromMsecs(benchmark::State& state) {
    for (auto _ : state) {
        DateTime t = DateTime::msecs(12345);
        benchmark::DoNotOptimize(t);
    }
}
BENCHMARK(BM_DateTimeFromMsecs);

// Benchmark elapsed time calculation pattern
static void BM_DateTimeElapsed(benchmark::State& state) {
    for (auto _ : state) {
        DateTime start = DateTime::now();
        // Simulate some work
        benchmark::ClobberMemory();
        DateTime end = DateTime::now();
        DateTime elapsed = end - start;
        benchmark::DoNotOptimize(elapsed);
    }
}
BENCHMARK(BM_DateTimeElapsed);
