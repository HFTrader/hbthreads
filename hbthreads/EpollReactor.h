#pragma once

#include "DateTime.h"
#include "Reactor.h"

namespace hbthreads {

//! An event dispatcher based on the Posix epoll mechanism
class EpollReactor : public Reactor {
public:
    //! Creates the epoll file descriptor
    //! timeout is the amount to block for events until `work()` returns
    EpollReactor(MemoryStorage* mem, DateTime timeout = DateTime::nsecs(-1));

    //! Closes the descriptor
    ~EpollReactor();

    //! Checks all the sockets currently monitored by the reactor
    //! and dispatches them accordingly.
    //! Returns false if something bad happened to the file descriptor
    bool work();

private:
    //! manages the socket
    void onSocketOps(int fd, Operation ops) override;

    DateTime _timeout;  //! How long should we block waiting for events?
    int _epollfd;       //! epoll file descriptor
};

}  // namespace hbthreads