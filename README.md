# hbthreads

A coroutine-based reactor library with a few unique features:
- simple to read for learning purposes: it uses **very basic** Modern C++ constructs that newcomers will find easy(ier) to grasp
- it has code complexity first and performance second as goals. We do not compromise on these.
- full documented, plenty of unit tests to learn
- minimal, voluntarily incomplete so it can be easily extended
- built-in intrusive pointer with integrated pool allocator (no new/malloc calls)
- uncomplicated coroutine semantics based on very basic (and fast) stack primitives
- event reactor with poll() and epoll() support (anyone still using select?)
- coroutines embedded as reactor dispatch mechanism

## Installation

### Prerequisites

- **C++14** compatible compiler (GCC 7+, Clang 5+)
- **CMake** 3.10 or higher
- **Boost** libraries (context, container, fiber)
- **pthread** (usually available on Linux)

### Installing Dependencies

**Ubuntu/Debian:**
```bash
sudo apt-get install cmake libboost-all-dev build-essential
```

**Fedora/RHEL:**
```bash
sudo dnf install cmake boost-devel gcc-c++
```

**Arch Linux:**
```bash
sudo pacman -S cmake boost gcc
```

**vcpkg:**
```bash
vcpkg install boost-context boost-container boost-fiber
```

### Building from Source

```bash
# Clone the repository
git clone https://github.com/HFTrader/hbthreads.git
cd hbthreads

# Configure and build
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel

# Run tests
ctest --test-dir build --output-on-failure

# Install (optional)
sudo cmake --install build
```

### Build Options

| Option | Default | Description |
|--------|---------|-------------|
| `BUILD_SHARED_LIBS` | ON | Build shared library (OFF for static) |
| `BUILD_TESTS` | ON | Build unit tests |
| `BUILD_DOCS` | ON | Build Doxygen documentation |
| `USE_LOCAL_BOOST` | OFF | Use locally compiled Boost (see `build_minimal_boost.sh`) |
| `HBTHREADS_USES_CLANG_TIDY` | OFF | Enable clang-tidy during compilation |
| `USE_SMALL_SIZE` | OFF | Use 16-bit size type for memory management |
| `USE_SMALL_COUNTER` | OFF | Use 16-bit counter for intrusive pointers |

Example with options:
```bash
cmake -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_SHARED_LIBS=OFF \
  -DBUILD_TESTS=ON
```

### Using in Your Project

After installation, use in your CMakeLists.txt:
```cmake
find_package(hbthreads REQUIRED)
target_link_libraries(your_target hbthreads)
```

Or add as a subdirectory:
```cmake
add_subdirectory(hbthreads)
target_link_libraries(your_target hbthreads)
```

## Quick Example

```cpp
#include <hbthreads/EpollReactor.h>
#include <hbthreads/LightThread.h>

using namespace hbthreads;

class MyThread : public LightThread {
public:
    void run() override {
        // Your coroutine logic here
        Event* ev = wait();  // Yield until event
        // Process event...
    }
};

int main() {
    boost::container::pmr::unsynchronized_pool_resource pool;
    storage = &pool;  // Set thread-local storage

    EpollReactor reactor(&pool);

    Pointer<MyThread> thread(new MyThread());
    thread->start(64 * 1024);  // 64KB stack

    reactor.monitor(some_fd, thread.get());

    while (reactor.active()) {
        reactor.work();
    }
}
```

See `tests/` directory for more comprehensive examples.

## Architecture

See [ARCHITECTURE.md](ARCHITECTURE.md) for detailed design documentation.

## Motivation

The primary motivation is to show that network/socket/coroutine programming does not need
to be necessarily complicated. I hope also to provide some learning material for my charity blog at
https://lucisqr.substack.com/

## License

This project is licensed under the [Boost Software License 1.0](LICENSE).

## TODO

Please check our [issues page on Github](https://github.com/HFTrader/hbthreads/issues) for our ever-changing list of requirements.
