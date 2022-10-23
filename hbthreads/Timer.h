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
    int fd;  //! The timerfd file descriptor
    Timer();
    bool start(DateTime interval);
    ~Timer();
};
}  // namespace hbthreads
