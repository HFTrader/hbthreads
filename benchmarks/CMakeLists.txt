# Benchmarks using Google Benchmark

add_executable(benchmarks
    main.cpp
    LightThreadBenchmarks.cpp
    PointerBenchmarks.cpp
    DateTimeBenchmarks.cpp
    HistogramBenchmarks.cpp
    ReactorBenchmarks.cpp
)

target_link_libraries(benchmarks
    benchmark::benchmark
    hbthreads
)

# Don't run benchmarks as part of ctest by default (they take too long)
# Users can run them manually: ./build/benchmarks/benchmarks
