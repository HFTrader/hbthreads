#include "Reactor.h"
#include <iostream>

using namespace hbthreads;

// We have to pass the memory storage to all the container to use,
// which is a pain but the price to pay for awesomeness
// Notice that the boost::container::flat* containers do not accept a memory
// Resource but an allocator so it is implicitly declared
Reactor::Reactor(MemoryStorage* mem) : _mem(mem), _socket_subs(mem), _thread_subs(mem) {
}

// All resources removed automagically
Reactor::~Reactor() {
}

bool Reactor::active() const {
    // no thread subscriptions, not active
    // this is typically used to terminate loops
    return !_socket_subs.empty();
}

void Reactor::monitor(int fd, LightThread* thread) {
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
    removeSubscriptions(fd);
}

void Reactor::removeSubscriptions(int fd) {
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
    // Get the first subscription for this thread
    ThreadSubscriptionSet::const_iterator first =
        _thread_subs.lower_bound(Subscription{-1, th});

    // Iterate until the end of the set or until the thread in the iterator
    // is a different one
    ThreadSubscriptionSet::const_iterator it = first;
    for (; (it != _thread_subs.end()) && (it->thread.get() == th); ++it) {
        int fd = it->fd;
        SocketSubscriberSet::iterator is = _socket_subs.find(Subscription{fd, th});
        if (is != _socket_subs.end()) {
            // This should return the next subscription in socket order
            is = _socket_subs.erase(is);

            // Find if this is the last socket and notify
            // It would be cleaner to just do another search but this is faster
            bool found = ((is != _socket_subs.end()) && (is->fd == fd)) ||
                         ((is != _socket_subs.begin()) && ((is - 1)->fd == fd));
            if (!found) {
                onSocketOps(it->fd, Operation::Removed);
            }
        }
    }
    // Erase the entire range
    _thread_subs.erase(first, it);
}

void Reactor::notifyEvent(int fd, EventType type) {
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
