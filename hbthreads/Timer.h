#pragma once

#include "DateTime.h"
#include <sys/timerfd.h>
#include <string.h>
#include <iostream>
#include <unistd.h>

namespace hbthreads {
//! Wraps a timerfd which is a file descriptor that will fire events
//! at a predefined interval. This is a kernel facility so you dont
//! have to keep a scheduler in parallel with epoll(). Convenient.
//! This class is not part of the library but of the examples.
struct Timer {
    // The constructor creates a file descriptor that is guaranteed
    // not to change through the lifetime of the object so it can be
    // added to a monitor right away after object creation.
    Timer();

    //! Starts the timer. Will fire at repeated intervals
    bool start(DateTime interval);

    //! Starts the timer. Will fire at intervals after a delay
    //! Absolute will make `delay` an absolute time instead of relative
    bool start(DateTime delay, DateTime interval, bool absolute = false);

    //! Starts the timer. Will fire at intervals after an absolute time
    bool oneShot(DateTime initial);

    //! Stops the timer
    bool stop();

    //! Closes the timer file descriptor if open
    ~Timer();

    //! returns the file descriptor
    int fd();

    //! Checks the timer for an event, returns immediately with 0 if no
    //! event exists. Returns -1 if an error occurred. Otherwise returns the number of
    //! expired events between reads - usually one
    int check() const;

private:
    int _fd;  //! The timerfd file descriptor
};
}  // namespace hbthreads
