#pragma once

#include "ImportedTypes.h"
#include "LightThread.h"

namespace hbthreads {

//! Base class for all reactor types - currently Epoll and Poll
//! A reactor or dispatcher is an object that watches over a pool of resources,
//! which in Unix is a collection of file descriptors, and accepts subscriptions
//! to it. When one of these resources show data availability, the reactor will
//! alert the subscribers. Note that the reactor will NOT read the data nor it
//! will make assumptions over the number of subscribers per resource.
//! This base Reactor class is virtual as it will not monitor the resources
//! itself, it is up to the derived classes to do it.
//! The purpose of this base class is only to manage subscriptions and dispatch
//! events properly to the light threads (coroutines)
//! TODO:
//! This reactor does not currently deal with OUT/WRITE notifications, which
//! are notifications that your application would like to get when there is
//! space to write data without blocking
class Reactor : public Object {
public:
    //! Takes a memory storage to allocate small objects. This storage can be
    //! thread-unsafe
    Reactor(MemoryStorage* resource);

    //! Releases all resources
    virtual ~Reactor();

    //! Set up one subscription. Duplicate subscriptions (same fd and thread)
    //! will be conflated in a set.
    void monitor(int fd, LightThread* thread);

    //! Remove all active subscriptions to this file descriptor
    void removeSocket(int fd);

    //! removes all subscriptions to the given thread
    void removeThread(LightThread* othread);

    //! Returns true if there is at least one single subscription active
    bool active() const noexcept;

private:
    //! removes all subscriptions to this thread
    void removeSubscriptions(LightThread* thread);

    //! removes all subscriptions to this file descriptor
    void removeSubscriptions(int fd);

protected:
    //! Type of operations valid on sockets
    enum class Operation : std::uint8_t { NA = 0, Added = 1, Removed = 2, Modified = 3 };

    //! The virtual interface
    //! Alert derived classes about one operation in a socket
    virtual void onSocketOps(int fd, Operation ops) = 0;

    //! notify all subscribers of this file descriptor about a read available
    void notifyEvent(int fd, EventType type);

    //! A specific operation on a specific file descriptor
    struct FileOps {
        int fd;            //! the file descriptor
        Operation action;  //! the operation
    };

    //! A connection between a file descriptor and a light thread
    struct Subscription {
        int fd;                       //! the file descriptor
        Pointer<LightThread> thread;  //! the light thread (coroutine)
    };

    //! A functor that will sort by file descriptor first and thread second
    struct SubscriptionBySocket {
        bool operator()(const Subscription& lhs, const Subscription& rhs) const {
            if (lhs.fd < rhs.fd) return true;
            if (lhs.fd > rhs.fd) return false;
            return lhs.thread.get() < rhs.thread.get();
        }
    };

    //! A functor that will sort by thread first and file descriptor second
    struct SubscriptionByThread {
        // Remember: nullptr is often but not guaranteed to be zero!
        // If this fails I suggest you reevaluate your career choices
        static_assert(nullptr == (void*)0,
                      "We need nullptr to be zero so this pointer works as an index");
        bool operator()(const Subscription& lhs, const Subscription& rhs) const {
            if (lhs.thread.get() < rhs.thread.get()) return true;
            if (lhs.thread.get() > rhs.thread.get()) return false;
            return lhs.fd < rhs.fd;
        }
    };

    //! A set of subscriptions ordered by file descriptor
    using SocketSubscriberSet = FlatSet<Subscription, SubscriptionBySocket>;

    //! A set of subscriptions ordered by thread
    using ThreadSubscriptionSet = FlatSet<Subscription, SubscriptionByThread>;

    MemoryStorage* _mem;               //! The memory resource where to allocate from
    SocketSubscriberSet _socket_subs;  //! Set of subscriptions ordered by file descriptor
    ThreadSubscriptionSet _thread_subs;  //! Set of subscriptions ordered by thread
};

}  // namespace hbthreads