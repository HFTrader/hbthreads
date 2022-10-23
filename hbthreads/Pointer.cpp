#include "Pointer.h"

namespace hbthreads {

// This is defined as `extern` in ImportedTypes.h such that it is up
// to `main()` (and each thread thereafter) to defined what exactly this is
__thread MemoryStorage* storage;
std::uint32_t ObjectCounter::_library_counter_size = sizeof(IntrusiveCounterType);
std::uint32_t ObjectCounter::_library_chunk_size = sizeof(IntrusiveSizeType);

}  // namespace hbthreads
