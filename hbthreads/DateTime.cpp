#include "DateTime.h"
#include <time.h>
#include <assert.h>

using namespace hbthreads;

DateTime DateTime::now(ClockType clock) {
    clockid_t cid = CLOCK_REALTIME;
    switch (clock) {
        case ClockType::RealTime: cid = CLOCK_REALTIME; break;
        case ClockType::Monotonic: cid = CLOCK_MONOTONIC; break;
    }
    timespec ts;
    int res = clock_gettime(cid, &ts);
    assert(res == 0);
    return DateTime::secs(ts.tv_sec) + DateTime::nsecs(ts.tv_nsec);
}
