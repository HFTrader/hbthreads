#include "DateTime.h"
#include "StringUtils.h"
#include <time.h>
#include <cstring>

#include <array>
#include <cstdio>

namespace hbthreads {
struct YearMonthDate {
    int16_t year;
    int16_t month;
    int16_t day;
    char str[8];
    uint32_t yyyymmdd;
    YearMonthDate() {
        year = month = day = yyyymmdd = 0;
        memset(str, 0, sizeof(str));
    }
    YearMonthDate(struct tm& tms) {
        year = tms.tm_year + 1900;
        month = tms.tm_mon + 1;
        day = tms.tm_mday;
        yyyymmdd = year * 10000 + month * 100 + day;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
        snprintf(str, sizeof(str), "%08u", yyyymmdd);
#pragma GCC diagnostic pop
    }
};

//! A table to translate from epoch to yyyy/mm/dd fast
//! Mainly used in DateTime::fromDate()
std::array<YearMonthDate, DateTime::MAXDATES> YMD_FROM_EPOCH;
std::array<std::array<std::array<time_t, 31>, 12>, DateTime::MAXYEARS> EPOCH_FROM_YMD;

struct DateTimeInitializer {
    DateTimeInitializer() {
        for (unsigned year = 0; year < DateTime::MAXYEARS; ++year) {
            for (unsigned month = 0; month < 12; ++month) {
                for (unsigned day = 0; day < 31; ++day) {
                    EPOCH_FROM_YMD[year][month][day] = 0;
                }
            }
        }
        memset(EPOCH_FROM_YMD.data(), 0, sizeof(EPOCH_FROM_YMD));
        for (unsigned j = 0; j < YMD_FROM_EPOCH.size(); ++j) {
            time_t epoch = time_t(j) * 86400;
            struct tm result;
            gmtime_r(&epoch, &result);
            YearMonthDate& ymd(YMD_FROM_EPOCH[j]);
            ymd = result;
            EPOCH_FROM_YMD[ymd.year - 1970][ymd.month - 1][ymd.day - 1] = epoch;
        }
    }
} datetime_initializer;

};  // namespace hbthreads

using namespace hbthreads;

DateTime DateTime::now(ClockType clock) {
    clockid_t cid = CLOCK_REALTIME;
    switch (clock) {
        case ClockType::RealTime: cid = CLOCK_REALTIME; break;
        case ClockType::Monotonic: cid = CLOCK_MONOTONIC; break;
    }
    timespec ts;
    __attribute__((unused)) int res = clock_gettime(cid, &ts);
    return DateTime::secs(ts.tv_sec) + DateTime::nsecs(ts.tv_nsec);
}

void DateTime::decompose(struct DecomposedTime& dectime) const {
    time_t epoch = secs();
    time_t days = epoch / SECONDS_IN_DAY;
    time_t seconds = epoch % SECONDS_IN_DAY;
    YearMonthDate& ymd(YMD_FROM_EPOCH[days]);
    dectime.nanos = (int)nanos();
    dectime.year = ymd.year;
    dectime.month = ymd.month;
    dectime.day = ymd.day;
    dectime.hour = seconds / 3600;
    dectime.minute = (seconds / 60) % 60;
    dectime.second = seconds % 60;
}

DateTime DateTime::round(DateTime interval) const {
    int64_t intns = interval.nsecs();
    if (intns == 0) return *this;
    if (intns < 0) intns = -intns;
    int64_t ns = epochns;
    int64_t rem = ns % intns;
    if (rem >= intns / 2) {
        rem -= intns;
    } else if (rem <= -intns / 2) {
        rem += intns;
    }
    return DateTime(rem);
}

bool DateTime::advance(DateTime time, DateTime interval) {
    if (time.epochns < epochns) {
        return false;
    }
    epochns += interval.epochns;
    if (time.epochns >= epochns) {
        auto num_intervals = (time.epochns) / interval.epochns + 1;
        epochns = num_intervals * interval.epochns;
    }
    return true;
}

int64_t DateTime::YYYYMMDD() const {
    int64_t date = epochns / NANOS_IN_DAY;
    return YMD_FROM_EPOCH[date].yyyymmdd;
}

DateTime DateTime::fromDate(int year, int month, int day) {
    time_t epoch = EPOCH_FROM_YMD[year - 1970][month - 1][day - 1];
    return DateTime::secs(epoch);
}

DateTime DateTime::fromTime(int hour, int minute, int second) {
    return DateTime::secs((hour * 60 + minute) * 60 + second);
}

char* DateTime::print(char* buf) const {
    // YYYYMMDD-HH:MM:SS.NNNNNNNNN
    // 000000000011111111112222222222
    // 012345678901234567890123456789
    DecomposedTime dec;
    decompose(dec);
    printpad<4>(buf, dec.year);
    printpad<2>(buf + 4, dec.month);
    printpad<2>(buf + 6, dec.day);
    printpad<2>(buf + 9, dec.hour);
    printpad<2>(buf + 12, dec.minute);
    printpad<2>(buf + 15, dec.second);
    printpad<9>(buf + 18, dec.nanos);
    buf[8] = '-';
    buf[11] = ':';
    buf[14] = ':';
    buf[17] = '.';
    return buf + 27;
}

char* DateTime::printTime(char* buf) const {
    // HH:MM:SS.NNNNNNNNN
    // 0000000000111111111
    // 0123456789012345678
    int64_t total_nanos = epochns % NANOS_IN_DAY;
    int64_t total_seconds = total_nanos / NANOS_IN_SECOND;
    int64_t hours = total_nanos / NANOS_IN_HOUR;
    int64_t minutes = (total_seconds / 60) % 60;
    int64_t seconds = total_seconds % 60;
    int64_t nanos = total_nanos % NANOS_IN_SECOND;
    printpad<2>(buf, hours);
    printpad<2>(buf + 3, minutes);
    printpad<2>(buf + 6, seconds);
    printpad<9>(buf + 9, nanos);
    buf[2] = ':';
    buf[5] = ':';
    buf[8] = '.';
    return buf + 18;
}

std::ostream& hbthreads::operator<<(std::ostream& out, const DateTime date) {
    if (date.nsecs() >= 0) {
        if (date.days() != 0) {
            char buf[32];
            char* ptr = date.print(buf);
            size_t len = ptr - buf;
            out.write(buf, len);
        } else {
            char buf[32];
            char* ptr = date.printTime(buf);
            size_t len = ptr - buf;
            out.write(buf, len);
        }
    } else {
        if (date.days() != 0) {
            out << DateTime();
        } else {
            out << "-";
            DateTime tmp = DateTime::nsecs(-date.nsecs());
            char buf[32];
            char* ptr = tmp.printTime(buf);
            size_t len = ptr - buf;
            out.write(buf, len);
        }
    }
    return out;
}
