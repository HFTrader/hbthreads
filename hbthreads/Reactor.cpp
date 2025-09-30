#include "Reactor.h"

using namespace hbthreads;

// We have to pass the memory storage to all the container to use,
// which is a pain but the price to pay for awesomeness
// Notice that the boost::container::flat* containers do not accept a memory
// Resource but an allocator so it is implicitly declared
Reactor::Reactor(MemoryStorage* mem) : _mem(mem), _socket_subs(mem), _thread_subs(mem) {
    assert(mem != nullptr && "MemoryStorage must not be null");
}

// All resources removed automagically
Reactor::~Reactor() {
}

bool Reactor::active() const noexcept {
    // no thread subscriptions, not active
    // this is typically used to terminate loops
    return !_socket_subs.empty();
}

void Reactor::monitor(int fd, LightThread* thread) {
    assert(fd >= 0 && "File descriptor must be valid");
    assert(thread != nullptr && "Thread must not be null");

    // Notify if this is the first socket we know about
    SocketSubscriberSet::const_iterator is =
        _socket_subs.lower_bound(Subscription{fd, nullptr});
    if ((is == _socket_subs.end()) || (is->fd != fd)) {
        onSocketOps(fd, Operation::Added);
    }

    // Insert relationships
    Subscription sub{fd, thread};
    _socket_subs.insert(sub);
    _thread_subs.insert(sub);
}

void Reactor::removeSocket(int fd) {
    assert(fd >= 0 && "File descriptor must be valid");
    removeSubscriptions(fd);
}

void Reactor::removeSubscriptions(int fd) {
    assert(fd >= 0 && "File descriptor must be valid");
    // Get the first subscription for this file descriptor
    SocketSubscriberSet::const_iterator first =
        _socket_subs.lower_bound(Subscription{fd, nullptr});

    // Iterate until the end of set or until the descriptor is a different one
    SocketSubscriberSet::const_iterator is = first;
    for (; (is != _socket_subs.end()) && (is->fd == fd); ++is) {
        _thread_subs.erase(Subscription{fd, is->thread});
    }
    // Erase all range of subscriptions
    _socket_subs.erase(first, is);
    onSocketOps(fd, Operation::Removed);
}

void Reactor::removeSubscriptions(LightThread* th) {
    assert(th != nullptr && "Thread must not be null");
    // Get the first subscription for this thread
    ThreadSubscriptionSet::const_iterator first =
        _thread_subs.lower_bound(Subscription{-1, th});

    // Track which FDs had their last subscription removed
    // Use a small inline set to avoid allocation for common case
    FlatSet<int> fds_to_remove(_mem);

    // Iterate until the end of the set or until the thread in the iterator
    // is a different one
    ThreadSubscriptionSet::const_iterator it = first;
    for (; (it != _thread_subs.end()) && (it->thread.get() == th); ++it) {
        int fd = it->fd;
        // Remove from socket subscriptions (we know it exists)
        _socket_subs.erase(Subscription{fd, th});

        // Check if this was the last subscription for this FD
        SocketSubscriberSet::const_iterator next_sub =
            _socket_subs.lower_bound(Subscription{fd, nullptr});
        if ((next_sub == _socket_subs.end()) || (next_sub->fd != fd)) {
            // No more subscriptions for this FD
            fds_to_remove.insert(fd);
        }
    }
    // Erase the entire range from thread subscriptions
    _thread_subs.erase(first, it);

    // Notify about removed sockets
    for (int fd : fds_to_remove) {
        onSocketOps(fd, Operation::Removed);
    }
}

void Reactor::notifyEvent(int fd, EventType type) {
    assert(fd >= 0 && "File descriptor must be valid");
    Event event;
    event.type = type;
    event.fd = fd;

    // Finds the first subscription to this file descriptor
    SocketSubscriberSet::const_iterator it =
        _socket_subs.lower_bound(Subscription{fd, nullptr});

    // Iterates over all subscriptions to this file descriptor
    using ThreadSet = FlatSet<Pointer<LightThread>>;
    ThreadSet removed(_mem);
    for (; (it != _socket_subs.end()) && (it->fd == fd); ++it) {
        if (!it->thread->resume(&event)) {
            // We cannot remove subscriptions here as it will
            // invalidate these iterators
            removed.insert(it->thread);
        }
    }
    for (const Pointer<LightThread>& th : removed) {
        removeSubscriptions(th.get());
    }
    if ((type == EventType::SocketError) || (type == EventType::SocketHangup)) {
        removeSubscriptions(fd);
    }
}
