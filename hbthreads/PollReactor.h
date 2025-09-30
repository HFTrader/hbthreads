#pragma once

#include "DateTime.h"
#include "Reactor.h"

#include <poll.h>

namespace hbthreads {

//! Uses ::poll() for waiting on a group of sockets.
//! This is here for completeness, education and enjoyment (mine).
//! In systems with epoll the use of ::poll() has no advantage really.
class PollReactor : public Reactor {
public:
    //! Initializes a reactor with a memory pool size
    //! The bigger the number, more waste but less frequent malloc() will be
    //! called. It depends on the number of sockets you will use.
    PollReactor(MemoryStorage* mem, DateTime timeout = DateTime::msecs(500));

    //! Checks if any of the subscribed sockets have events on them.
    void work();

private:
    //! Called when operations (add,remove,modify) are performed on a socket
    void onSocketOps(int fd, Operation ops) override;

    //! Rebuilds the poll vector `_fds` from the _sockets set.
    //! This implements a delayed rebuild pattern: multiple socket add/remove
    //! operations set the _dirty flag, and rebuild() is only called once
    //! before work(). This batches socket operations and avoids rebuilding
    //! the dense array required by poll() on every socket change.
    void rebuild();

    //! Amount to block waiting for events
    DateTime _timeout;

    //! Saves typing
    using PollVector = SmallVector<pollfd, 16>;

    //! The vector contains the array used in calling `::poll()`
    PollVector _fds;

    //! Keeps track of all existing sockets (sparse set)
    FlatSet<int> _sockets;

    //! Dirty flag indicates _fds needs rebuild before next poll().
    //! This allows batching multiple socket operations without
    //! rebuilding the dense _fds array on every change.
    bool _dirty;
};

}  // namespace hbthreads