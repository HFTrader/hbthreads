#include "EpollReactor.h"
#include "SocketUtils.h"
#include <array>
// One the hidden great things about epoll is that you do not need to include
// its header in the class header file like with poll()
#include <sys/epoll.h>

using namespace hbthreads;

// Passes the memory storage down to the Reactor base
EpollReactor::EpollReactor(MemoryStorage* mem, DateTime timeout) : Reactor(mem) {
    _timeout = timeout;
    _epollfd = ::epoll_create1(0);
    if (_timeout.nsecs() == 0) {
        setSocketNonBlocking(_epollfd);
    }
}

// Closes the file descriptor if open
// Remember that zero is a valid file descriptor
EpollReactor::~EpollReactor() {
    if (_epollfd >= 0) ::close(_epollfd);
}

bool EpollReactor::work() {
    if (_epollfd < 0) return false;

    // Handles pending events
    std::array<epoll_event, 16> events;
    int nd = ::epoll_wait(_epollfd, events.data(), events.size(), _timeout.msecs());
    if (nd < 0) {
        // This should never happen but it is possible
        perror("EpollReactor::work() on epoll_wait");
        return false;
    }

    // If there are events, dispatch them
    for (int j = 0; j < nd; ++j) {
        const epoll_event& ev(events[j]);
        // Notify reads
        if ((ev.events & EPOLLIN) != 0) {
            notifyEvent(ev.data.fd, EventType::SocketRead);
        }
        // Notify errors
        if ((ev.events & (EPOLLERR)) != 0) {
            notifyEvent(ev.data.fd, EventType::SocketError);
        }
        // Notify hangup
        if ((ev.events & (EPOLLHUP)) != 0) {
            notifyEvent(ev.data.fd, EventType::SocketHangup);
        }
    }
    return true;
}

void EpollReactor::onSocketOps(int fd, Operation ops) {
    // Sanity check
    if (_epollfd < 0) return;

    // Cant have local vars inside the switch
    epoll_event event{};
    int res;

    switch (ops) {
        case Operation::Added:
            // Tells the kernel about this new socket so we get notified
            event.data.fd = fd;
            event.events = EPOLLIN | EPOLLRDHUP | EPOLLPRI | EPOLLERR;
            res = epoll_ctl(_epollfd, EPOLL_CTL_ADD, fd, &event);
            if (res != 0) {
                perror("EpollReactor::onSocketOps() on epoll_ctl(ADD)");
            }
            break;
        case Operation::Removed:
            // Tells the kernel we do not need to be notified by this socket anymore
            res = epoll_ctl(_epollfd, EPOLL_CTL_DEL, fd, nullptr);
            if (res != 0) {
                if (errno != EBADF) {
                    perror("EpollReactor::onSocketOps() on epoll_ctl(DEL)");
                }
            }
            break;
        case Operation::Modified:
        case Operation::NA: break;
    }
}
