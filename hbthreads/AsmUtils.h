// AsmUtils.h - Assembly utility functions for low-level operations
//
// This header provides inline assembly utilities for performance-critical
// operations, particularly timing-related functions using CPU hardware counters.

#pragma once

#include <cstdint>

namespace hbthreads {

// Reads the Time Stamp Counter (TSC) register
//
// This function provides a high-resolution timestamp by reading the x86
// Time Stamp Counter (TSC) register. The TSC is a 64-bit register that counts
// processor clock cycles since reset, providing sub-microsecond timing precision.
//
// Returns: Current TSC value as a 64-bit unsigned integer
//
// Note: This function is x86-specific and requires a processor that supports
// the RDTSC instruction. The returned value is not guaranteed to be
// monotonic across CPU frequency changes or system suspend/resume.
static inline uint64_t tic() {
    return __builtin_ia32_rdtsc();
}

}  // namespace hbthreads
