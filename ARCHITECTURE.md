# Architecture

This document describes the design and architecture of hbthreads, a coroutine-based reactor library for event-driven programming.

## Overview

hbthreads provides three core abstractions:

1. **LightThread** - Stack-full coroutines for cooperative multitasking
2. **Reactor** - Event dispatchers that monitor file descriptors
3. **Pointer** - Intrusive smart pointers with custom memory allocation

```
┌─────────────────────────────────────────────────────────────┐
│                      Application                             │
├─────────────────────────────────────────────────────────────┤
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐       │
│  │ LightThread  │  │ LightThread  │  │ LightThread  │       │
│  │  (coroutine) │  │  (coroutine) │  │  (coroutine) │       │
│  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘       │
│         │                 │                 │                │
│         └────────────┬────┴─────────────────┘                │
│                      ▼                                       │
│         ┌────────────────────────┐                          │
│         │        Reactor         │                          │
│         │  (EpollReactor or      │                          │
│         │   PollReactor)         │                          │
│         └───────────┬────────────┘                          │
│                     ▼                                        │
│         ┌────────────────────────┐                          │
│         │   File Descriptors     │                          │
│         │  (sockets, pipes, etc) │                          │
│         └────────────────────────┘                          │
└─────────────────────────────────────────────────────────────┘
```

## Core Components

### LightThread

`LightThread` implements stack-full coroutines using Boost.Context. Unlike stackless coroutines (C++20 coroutines), stack-full coroutines can yield from any call depth.

**Key characteristics:**
- Each thread has its own stack (configurable size)
- Context switching via `wait()` and `resume()`
- Managed by intrusive reference counting

**Lifecycle:**
```
┌─────────┐     start()     ┌─────────┐
│ Created │ ───────────────▶│ Running │
└─────────┘                 └────┬────┘
                                 │
                    wait()       │       resume()
                    ┌────────────┴────────────┐
                    ▼                         │
              ┌──────────┐                    │
              │ Waiting  │────────────────────┘
              └──────────┘
                    │
                    │ run() returns
                    ▼
              ┌──────────┐
              │ Finished │
              └──────────┘
```

**Usage pattern:**
```cpp
class MyThread : public LightThread {
    void run() override {
        while (running) {
            Event* ev = wait();  // Yield to reactor
            // Handle event...
        }
    }
};
```

### Reactor

The `Reactor` base class manages subscriptions between file descriptors and LightThreads. Two implementations are provided:

| Class | Mechanism | Best For |
|-------|-----------|----------|
| `EpollReactor` | Linux epoll | Production, high connection count |
| `PollReactor` | POSIX poll | Portability, learning |

**Subscription model:**
- Multiple threads can subscribe to the same file descriptor
- A thread can subscribe to multiple file descriptors
- Subscriptions are stored in sorted flat sets for cache efficiency

**Event flow:**
```
1. Application calls reactor.work()
2. Reactor blocks on epoll_wait()/poll()
3. OS signals file descriptor readiness
4. Reactor looks up subscribed threads
5. Reactor calls thread->resume(event)
6. Thread wakes from wait(), processes event
7. Thread calls wait() again or exits
```

### Pointer<T> and Memory Management

The `Pointer<T>` template provides intrusive reference counting with custom allocation:

**Design goals:**
- Zero-allocation steady state (pool allocator)
- Minimal memory overhead (configurable counter size)
- Efficient deallocation (size embedded in allocation)

**Class hierarchy:**
```
ObjectCounter (reference counting + custom new/delete)
      │
      ▼
   Object (virtual inheritance base)
      │
      ├──▶ LightThread
      └──▶ Reactor
```

**Memory layout:**
```
┌──────────────────────────────────────────┐
│ size (2-4 bytes) │ Object data...        │
└──────────────────────────────────────────┘
                   ▲
                   │
            Pointer returned to user
```

**Thread-local storage:**
```cpp
// Must be initialized before creating objects
extern __thread MemoryStorage* storage;

// Typical initialization
boost::container::pmr::unsynchronized_pool_resource pool;
storage = &pool;
```

## Supporting Components

### DateTime

High-resolution time representation using nanoseconds internally. Provides conversions and arithmetic operations.

### Timer

Timer management integrated with reactors using timerfd (Linux).

### SocketUtils

Helper functions for socket creation, configuration (non-blocking, TCP_NODELAY, etc.), and multicast setup.

### Histogram

Lock-free histogram for latency measurements with percentile calculations.

### TSC (Time Stamp Counter)

CPU timestamp counter utilities for low-latency timing.

### BufferPrinter

Allocation-free string formatting for use in malloc hooks.

## Design Principles

### 1. Simplicity Over Abstraction

The codebase intentionally avoids complex template metaprogramming and deep inheritance hierarchies. Each class has a clear, focused purpose.

### 2. Zero-Allocation Steady State

After initial setup, the event loop should not allocate memory. This is achieved through:
- Pool allocators for all dynamic objects
- Flat containers (contiguous memory)
- Pre-allocated event buffers

### 3. Cache-Friendly Data Structures

- `FlatSet` and `FlatMap` use contiguous storage
- `SmallVector` avoids heap allocation for small sizes
- Subscription lookups are O(log n) with good cache behavior

### 4. Explicit Resource Management

- No hidden allocations
- Clear ownership via intrusive pointers
- Deterministic destruction

## Threading Model

hbthreads is designed for **single-threaded** event loops:

- One reactor per thread
- `MemoryStorage` is thread-local (unsynchronized)
- No locks in the hot path

For multi-core systems, run separate reactors on separate threads with workload partitioning.

## Performance Considerations

### HFT Optimizations (Release builds)

The CMake configuration enables aggressive optimizations:
- `-march=native -mtune=native` (CPU-specific instructions)
- `-flto` (link-time optimization)
- `-O3 -ffast-math` (aggressive optimization)
- `-fomit-frame-pointer` (more registers)

### Memory Optimization Flags

For memory-constrained environments:
- `USE_SMALL_SIZE=ON` - 16-bit allocation size (max 64KB objects)
- `USE_SMALL_COUNTER=ON` - 16-bit reference counter (max 65535 refs)

## File Organization

```
hbthreads/
├── hbthreads/           # Library source
│   ├── LightThread.h/cpp    # Coroutine implementation
│   ├── Reactor.h/cpp        # Base reactor class
│   ├── EpollReactor.h/cpp   # epoll-based reactor
│   ├── PollReactor.h/cpp    # poll-based reactor
│   ├── Pointer.h/cpp        # Smart pointer + memory
│   ├── ImportedTypes.h      # Boost type aliases
│   ├── DateTime.h/cpp       # Time utilities
│   ├── Timer.h/cpp          # Timer management
│   ├── SocketUtils.h/cpp    # Socket helpers
│   ├── *UnitTests.cpp       # Unit tests
│   └── ...
├── tests/               # Example programs
│   ├── tcptest.cpp          # TCP server/client example
│   ├── udptest.cpp          # UDP multicast example
│   ├── coroexample.cpp      # Coroutine comparison
│   └── ...
├── CMakeLists.txt       # Build configuration
├── README.md            # Project overview
├── ARCHITECTURE.md      # This document
└── LICENSE              # Boost Software License
```

## Extending hbthreads

### Adding a New Reactor

1. Inherit from `Reactor`
2. Implement `onSocketOps(int fd, Operation ops)`
3. Implement `work()` method calling `notifyEvent()`

### Adding Custom Objects

1. Inherit from `Object`
2. Use `Pointer<YourClass>` for management
3. Ensure `storage` is initialized before allocation

## Known Limitations

- **Linux only** - Uses epoll, eventfd, timerfd
- **No write notifications** - Reactor only handles read events (TODO)
- **Single-threaded** - No built-in thread safety
- **No signal handling** - Must be handled externally
