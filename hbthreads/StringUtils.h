#pragma once
#include <cstdint>
#include <ostream>
#include <string>

namespace hbthreads {

void printhex(std::ostream& out, const void* data, uint32_t size,
              const std::string& line_prefix, int NUMITEMS);

}  // namespace hbthreads