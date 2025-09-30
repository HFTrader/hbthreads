#include "TSC.h"

namespace hbthreads {

// Initialize static members
uint64_t TSC::_ns_per_tick_num = 1;
uint64_t TSC::_ns_per_tick_denom = 1;

}  // namespace hbthreads