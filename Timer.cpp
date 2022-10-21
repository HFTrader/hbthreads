#include "Timer.h"

using namespace hbthreads;

Timer::Timer() {
    fd = -1;
}

bool Timer::start(DateTime interval) {
    // Create a non-blocking descriptor
    fd = ::timerfd_create(CLOCK_REALTIME, TFD_NONBLOCK);
    if (fd < 0) {
        perror("Timer::start(): timerfd_create");
    }

    int64_t secs = interval.secs();
    int64_t nsecs = (interval - DateTime::secs(secs)).nsecs();

    // Set to 100 ms interval notification
    itimerspec ts;
    memset(&ts, 0, sizeof(ts));
    ts.it_interval.tv_sec = secs;
    ts.it_interval.tv_nsec = nsecs;
    ts.it_value.tv_sec = secs;
    ts.it_value.tv_nsec = nsecs;
    int res = ::timerfd_settime(fd, 0, &ts, nullptr);
    if (res != 0) {
        perror("Timer::start: Could not set the alarm:");
        return false;
    }
    return true;
}

Timer::~Timer() {
    // Close on destroy
    if (fd >= 0) ::close(fd);
}
