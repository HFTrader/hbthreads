#pragma once

#include <cstdint>

static inline uint64_t tic() {
    return __builtin_ia32_rdtsc();
}