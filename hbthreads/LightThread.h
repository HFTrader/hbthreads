#pragma once

#include "ImportedTypes.h"
#include "Pointer.h"
#include <cstdint>

namespace hbthreads {

//! Events that can be returned to the threads
enum class EventType : uint16_t {
    NA = 0,
    SocketRead = 1,
    SocketWriteable = 2,  // this is not currently implemented or handled
    SocketError = 3,
    SocketHangup = 4
};

//! Event object returned to a waiting thread
struct Event {
    EventType type;
    union {
        int fd;
    };
};

//! Technically a stack-full coroutine that can be held in an intrusive pointer
//! This is an abstract class. You need to subclass and implement the `run()`
//! virtual method.
class LightThread : public Object {
public:
    //! Constructs but does not initialize
    LightThread();

    //! Destructs and deallocates stack
    ~LightThread();

    //! Passes control to the caller, usually a Reactor
    //! Returns when the reactor has events this threads subscribed to.
    Event* wait();

    //! Resumes operation. Typically the thread would be blocked at `wait()`
    bool resume(Event* event);

    //! Where you place your good stuff
    virtual void run() = 0;

    //! Allocates the thread stack and starts executing the `run()` call
    void start(size_t stack_size);

private:
    //! Entry point from the coroutine context. Called by `start()`
    static void entry(transfer_t t);

    //! Contains saved registers for context switch
    ThreadContext _ctx;

    //! Holds information about who called this thread last
    ReturnContext _ret;

    //! the allocated stack
    stack_context _stack;

    //! the requested stack size, can be different from `_stack.size`
    size_t _stack_size;
};

}  // namespace hbthreads
