#include "LightThread.h"
#include "Reactor.h"
#include <iostream>

// You see how easy coroutines are once you strip down all the logic
// from all these libraries. This is a strip naked implementation
using namespace hbthreads;

LightThread::LightThread() {
}

LightThread::~LightThread() {
    // Deallocate stack
    StackAllocator sa(_stack_size);
    sa.deallocate(_stack);
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

    // Once the virtual loop ends, returns zero (null) so the caller is notified
    // that this thread is done. This is checked in LightThread::resume() below
    while (true) {
        th->_ret = jump_fcontext(th->_ret.fctx, 0);
    }
}

bool LightThread::resume(Event* event) {
    // Will jump back on where the thread left, typically after the
    // first line in `wait()`
    _ret = jump_fcontext(_ret.fctx, event);

    // Returns false if the thread returns zero. See comments in `entry()` above
    return (_ret.data != 0);
}

void LightThread::start(size_t stack_size) {
    _stack_size = stack_size;
    // This is currently defined as a protected stack allocator in ImportedTypes.h
    StackAllocator sa(_stack_size);
    _stack = sa.allocate();

    // Assigns the stack and prepares for the jump
    _ctx = make_fcontext(_stack.sp, _stack.size, LightThread::entry);

    // This will call LightThread::entry() and execute the virtual `run()` call
    // passing the pointer to this object as a parameter
    _ret = jump_fcontext(_ctx, (void*)this);
}
