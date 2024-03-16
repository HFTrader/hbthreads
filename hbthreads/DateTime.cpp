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
    __attribute__((unused)) int res = clock_gettime(cid, &ts);
    assert(res == 0);
    return DateTime::secs(ts.tv_sec) + DateTime::nsecs(ts.tv_nsec);
}

void DateTime::decompose(struct DecomposedTime& dectime) const {
    static const int BASE_YEAR = 1900;
    time_t epoch = secs();
    struct tm result;
    gmtime_r(&epoch, &result);
    dectime.nanos = (int)nanos();
    dectime.year = result.tm_year + BASE_YEAR;
    dectime.month = result.tm_mon + 1;
    dectime.day = result.tm_mday;
    dectime.hour = result.tm_hour;
    dectime.minute = result.tm_min;
    dectime.second = result.tm_sec;
}
