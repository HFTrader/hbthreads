// Benchmarks for Histogram operations

#include <benchmark/benchmark.h>
#include "Histogram.h"
#include <random>

using namespace hbthreads;

// Benchmark histogram add operation
static void BM_HistogramAdd(benchmark::State& state) {
    Histogram<1000> hist(0.0, 1000.0);
    double value = 500.0;

    for (auto _ : state) {
        hist.add(value);
        benchmark::DoNotOptimize(hist.count());
    }
}
BENCHMARK(BM_HistogramAdd);

// Benchmark histogram add with varying bin counts
static void BM_HistogramAdd_BinCount(benchmark::State& state) {
    const int bins = state.range(0);
    double value = 500.0;

    // Use different histogram sizes based on parameter
    if (bins == 100) {
        Histogram<100> hist(0.0, 1000.0);
        for (auto _ : state) {
            hist.add(value);
            benchmark::DoNotOptimize(hist.count());
        }
    } else if (bins == 1000) {
        Histogram<1000> hist(0.0, 1000.0);
        for (auto _ : state) {
            hist.add(value);
            benchmark::DoNotOptimize(hist.count());
        }
    } else if (bins == 10000) {
        Histogram<10000> hist(0.0, 1000.0);
        for (auto _ : state) {
            hist.add(value);
            benchmark::DoNotOptimize(hist.count());
        }
    }
}
BENCHMARK(BM_HistogramAdd_BinCount)
    ->Arg(100)
    ->Arg(1000)
    ->Arg(10000);

// Benchmark histogram with random values
static void BM_HistogramAddRandom(benchmark::State& state) {
    Histogram<1000> hist(0.0, 1000.0);
    std::mt19937 rng(42);
    std::uniform_real_distribution<double> dist(0.0, 1000.0);

    for (auto _ : state) {
        hist.add(dist(rng));
        benchmark::DoNotOptimize(hist.count());
    }
}
BENCHMARK(BM_HistogramAddRandom);

// Benchmark histogram percentile calculation
static void BM_HistogramPercentile(benchmark::State& state) {
    Histogram<1000> hist(0.0, 1000.0);

    // Pre-populate histogram
    std::mt19937 rng(42);
    std::uniform_real_distribution<double> dist(0.0, 1000.0);
    for (int i = 0; i < 10000; i++) {
        hist.add(dist(rng));
    }

    for (auto _ : state) {
        double p50 = hist.percentile(50);
        benchmark::DoNotOptimize(p50);
    }
}
BENCHMARK(BM_HistogramPercentile);

// Benchmark histogram summary calculation
static void BM_HistogramSummary(benchmark::State& state) {
    Histogram<1000> hist(0.0, 1000.0);

    // Pre-populate histogram
    std::mt19937 rng(42);
    std::uniform_real_distribution<double> dist(0.0, 1000.0);
    for (int i = 0; i < 10000; i++) {
        hist.add(dist(rng));
    }

    for (auto _ : state) {
        Stats stats = hist.summary();
        benchmark::DoNotOptimize(stats);
    }
}
BENCHMARK(BM_HistogramSummary);

// Benchmark histogram reset
static void BM_HistogramReset(benchmark::State& state) {
    Histogram<1000> hist(0.0, 1000.0);

    for (auto _ : state) {
        state.PauseTiming();
        // Add some data
        for (int i = 0; i < 100; i++) {
            hist.add(i * 10.0);
        }
        state.ResumeTiming();

        hist.reset();
        benchmark::DoNotOptimize(hist.count());
    }
}
BENCHMARK(BM_HistogramReset);

// Benchmark typical latency tracking pattern
static void BM_HistogramLatencyPattern(benchmark::State& state) {
    Histogram<1000> hist(0.0, 10000.0);  // 0-10ms in microseconds
    std::mt19937 rng(42);
    // Simulate typical latency distribution (log-normal-ish)
    std::normal_distribution<double> dist(100.0, 50.0);

    for (auto _ : state) {
        double latency = std::abs(dist(rng));
        hist.add(latency);

        // Occasionally compute stats (every 1000 samples)
        if (hist.count() % 1000 == 0) {
            Stats stats = hist.summary();
            benchmark::DoNotOptimize(stats);
        }
    }
}
BENCHMARK(BM_HistogramLatencyPattern);
