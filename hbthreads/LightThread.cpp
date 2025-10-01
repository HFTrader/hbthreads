#include "LightThread.h"
#include "Reactor.h"

// You see how easy coroutines are once you strip down all the logic
// from all these libraries. This is a strip naked implementation
using namespace hbthreads;

LightThread::LightThread() {
}

LightThread::~LightThread() {
    // Deallocate stack if it was allocated
    // Note: No check for running thread - assumes proper lifecycle management
    if (_stack_size > 0) {
        StackAllocator sa(_stack_size);
        sa.deallocate(_stack);
    }
}

Event* LightThread::wait() {
    // jumps back to the caller. Saves the return address upon return
    _ret = jump_fcontext(_ret.fctx, this);

    // The event pointer is passed on the saved transfer field
    return reinterpret_cast<Event*>(_ret.data);
}

void LightThread::entry(transfer_t ctx) {
    // This will get called by the coroutine infrastructure after jump_context call below
    // The pointer to the object is passed as a context data
    LightThread* th = reinterpret_cast<LightThread*>(ctx.data);

    // Saves the return address in the object itself so it knows where to go
    // once wait() is called
    th->_ret = ctx;

    // Starts the virtual loop
    th->run();

    // Once the virtual loop ends, return null to signal thread completion
    // The infinite loop was removed as it's unnecessary and wasteful
    th->_ret = jump_fcontext(th->_ret.fctx, nullptr);
}

bool LightThread::resume(Event* event) {
    // Will jump back to where the thread left, typically after the
    // first line in wait()
    _ret = jump_fcontext(_ret.fctx, event);

    // Returns true if thread yielded (sent non-null data), false if completed (sent null)
    return (_ret.data != nullptr);
}

void LightThread::start(size_t stack_size) {
    // Prevent double initialization
    if (_stack_size > 0) {
        return;  // Already started
    }

    _stack_size = stack_size;

    // Allocate stack for this thread
    StackAllocator sa(_stack_size);
    _stack = sa.allocate();

    // Create execution context on the allocated stack
    _ctx = make_fcontext(_stack.sp, _stack.size, LightThread::entry);

    // Jump to the coroutine entry point, passing this object as context
    _ret = jump_fcontext(_ctx, (void*)this);
}
