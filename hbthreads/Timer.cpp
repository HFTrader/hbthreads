#include "Timer.h"

using namespace hbthreads;

Timer::Timer() {
    _fd = -1;
}

static int createTimer() {
    // Create a non-blocking descriptor
    int fd = ::timerfd_create(CLOCK_REALTIME, TFD_NONBLOCK);
    if (fd < 0) {
        perror("Timer::start(): timerfd_create");
    }
    return fd;
}

static timespec asTimespec(DateTime interval) {
    int64_t secs = interval.secs();
    int64_t nsecs = (interval - DateTime::secs(secs)).nsecs();
    timespec ts;
    ts.tv_sec = secs;
    ts.tv_nsec = nsecs;
    return ts;
}

int Timer::fd() {
    if (_fd < 0) _fd = createTimer();
    return _fd;
}

bool Timer::start(DateTime delay, DateTime interval, bool absolute) {
    if (_fd < 0) {
        _fd = createTimer();
    }
    itimerspec ts;
    ts.it_value = asTimespec(delay);
    ts.it_interval = asTimespec(interval);
    int flags = absolute ? TFD_TIMER_ABSTIME : 0;
    int res = ::timerfd_settime(_fd, flags, &ts, nullptr);
    if (res != 0) {
        perror("Timer::start: Could not set the alarm:");
        return false;
    }
    return true;
}

bool Timer::start(DateTime interval) {
    return start(interval, interval, false);
}

bool Timer::oneShot(DateTime initial) {
    return start(initial, DateTime::zero(), true);
}

bool Timer::stop() {
    return start(DateTime::zero(), DateTime::zero(), false);
}

Timer::~Timer() {
    // Close on destroy
    if (_fd >= 0) ::close(_fd);
}
