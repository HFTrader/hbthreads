// LightThread.h - Stack-full coroutine implementation for event-driven programming
//
// This header defines a coroutine-based threading model using Boost.Context for
// efficient context switching. LightThread provides cooperative multitasking where
// threads yield control back to a reactor when waiting for I/O events.
//
// Key features:
// - Stack-full coroutines with configurable stack sizes
// - Efficient context switching using Boost.Context
// - Event-driven programming model with wait/resume semantics
// - Automatic stack management and cleanup
// - Integration with intrusive pointer system for memory management

#pragma once

#include "ImportedTypes.h"
#include "Pointer.h"
#include <cstdint>

namespace hbthreads {

// Event types that can be delivered to waiting threads
// Used by reactors to notify threads of I/O readiness or errors
enum class EventType : uint16_t {
    NA = 0,              // Not applicable/uninitialized
    SocketRead = 1,      // Socket has data available for reading
    SocketWriteable = 2, // Socket is ready for writing (not currently implemented)
    SocketError = 3,     // Socket error occurred
    SocketHangup = 4     // Socket connection closed/hung up
};

// Event structure passed to resumed threads
// Contains the event type and associated data (currently only file descriptor)
struct Event {
    EventType type;
    union {
        int fd;  // File descriptor associated with the event
    };
};

// Stack-full coroutine class providing cooperative multitasking
// This is an abstract base class that must be subclassed to implement the run() method.
// Threads are managed by a Reactor which handles I/O events and resumes threads accordingly.
//
// Thread lifecycle:
// 1. Create subclass and implement run()
// 2. Call start() to allocate stack and begin execution
// 3. Call wait() to yield control back to reactor
// 4. Reactor calls resume() when events are ready
// 5. Thread automatically cleans up when destroyed
class LightThread : public Object {
public:
    // Default constructor - creates uninitialized thread
    // Stack allocation happens in start()
    LightThread();

    // Destructor - deallocates thread stack if allocated
    ~LightThread();

    // Yield control back to the calling reactor
    // Returns when reactor has events this thread subscribed to
    // The returned Event pointer contains details about what triggered the resume
    Event* wait();

    // Resume thread execution after it called wait()
    // Typically called by a Reactor when I/O events are ready
    // Returns true if thread is still active, false if thread completed
    bool resume(Event* event);

    // Pure virtual method that contains the thread's main logic
    // Implement this in subclasses to define thread behavior
    // Call wait() to yield control when waiting for I/O
    virtual void run() = 0;

    // Initialize and start the thread with specified stack size
    // Allocates stack memory and begins execution of run() method
    // Must be called before resume() can be used
    void start(size_t stack_size);

private:
    // Static entry point called by Boost.Context when thread starts
    // Sets up the coroutine context and calls the virtual run() method
    static void entry(transfer_t t);

    // Saved execution context for context switching
    ThreadContext _ctx;

    // Return context information for resuming execution
    ReturnContext _ret;

    // Allocated stack memory for this thread
    stack_context _stack;

    // Requested stack size (may differ from actual allocated size)
    size_t _stack_size;
};

}  // namespace hbthreads
