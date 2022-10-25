#pragma once

#include <cstdint>

namespace hbthreads {
static inline uint64_t tic() {
    return __builtin_ia32_rdtsc();
}
}  // namespace hbthreads
