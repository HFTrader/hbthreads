#pragma once
#include <cstdint>
#include <ostream>
#include <string>

namespace hbthreads {

void printhex(std::ostream& out, const void* data, uint32_t size,
              const std::string& line_prefix, int NUMITEMS);

template <size_t N>
char* printpad(char* ptr, uint32_t value) {
    for (size_t j = 0; j < N; ++j) {
        ptr[N - j - 1] = (value % 10) + '0';
        value = value / 10;
    }
    return ptr + N;
}

}  // namespace hbthreads