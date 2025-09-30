#include <gtest/gtest.h>
#include "DateTime.h"

using namespace hbthreads;

TEST(DateTime, Constructor) {
    DateTime dt;
    EXPECT_EQ(dt.nsecs(), 0);
}

TEST(DateTime, Seconds) {
    DateTime tm = DateTime::secs(1);
    EXPECT_EQ(tm.secs(), 1);
    EXPECT_EQ(tm.msecs(), 1000);
    EXPECT_EQ(tm.usecs(), 1000000);
    EXPECT_EQ(tm.nsecs(), 1000000000);
}

TEST(DateTime, Milliseconds) {
    DateTime tm = DateTime::msecs(1);
    EXPECT_EQ(tm.secs(), 0);
    EXPECT_EQ(tm.msecs(), 1);
    EXPECT_EQ(tm.usecs(), 1000);
    EXPECT_EQ(tm.nsecs(), 1000000);
}

TEST(DateTime, MicroSeconds) {
    DateTime tm = DateTime::usecs(1);
    EXPECT_EQ(tm.secs(), 0);
    EXPECT_EQ(tm.msecs(), 0);
    EXPECT_EQ(tm.usecs(), 1);
    EXPECT_EQ(tm.nsecs(), 1000);
}

TEST(DateTime, OperatorAdd) {
    DateTime tm = DateTime::secs(1) + DateTime::secs(2);
    EXPECT_EQ(tm.secs(), 3);
    EXPECT_EQ(tm.nsecs(), 3000000000);
}

TEST(DateTime, OperatorSub) {
    DateTime tm = DateTime::secs(1) - DateTime::secs(2);
    EXPECT_EQ(tm.secs(), -1);
    EXPECT_EQ(tm.nsecs(), -1000000000);
}

TEST(DateTime, decomposeTime_Basic) {
    struct Test {
        int64_t epoch;
        int64_t date;
    };
    std::vector<Test> tests = {{0, 19700101},
                               {315532800, 19800101},
                               {1710801127, 20240318},
                               {2145920400, 20380101}};
    for (Test test : tests) {
        DateTime date = DateTime::secs(test.epoch);
        DecomposedTime dectime;
        date.decompose(dectime);
        int64_t result = dectime.year * 10000 + dectime.month * 100 + dectime.day;
        EXPECT_EQ(test.date, result);
        EXPECT_EQ(result, date.YYYYMMDD());
        DateTime recdate = DateTime::fromDate(dectime.year, dectime.month, dectime.day);
        EXPECT_EQ(date.date(), recdate);
    }
}

TEST(DateTime, decomposeTime_Extensive) {
    for (time_t epoch : {0, 315532800, 1710800514, 2145920400}) {
        for (int j = -3600 * 48; j < 3600 * 48; ++j) {
            if (epoch < -j) continue;
            DateTime date = DateTime::secs(epoch + j);

            DecomposedTime dectime;
            date.decompose(dectime);
            int64_t ymd = dectime.year * 10000 + dectime.month * 100 + dectime.day;
            EXPECT_EQ(date.YYYYMMDD(), ymd);

            DateTime result =
                DateTime::fromDate(dectime.year, dectime.month, dectime.day) +
                DateTime::fromTime(dectime.hour, dectime.minute, dectime.second);

            EXPECT_EQ(result, date);
        }
    }
}

TEST(DateTime, round_perfect) {
    const DateTime INTERVAL = DateTime::minutes(30);
    std::vector<DateTime> TEST_DATES = {
        DateTime::fromDate(1970, 1, 1) - DateTime::hours(1),  // negative date
        DateTime::fromDate(1970, 1, 1),                       // zero
        DateTime::fromDate(1980, 1, 1)};                      // positive
    for (DateTime base_date : TEST_DATES) {
        for (int64_t j = -30; j < -15; ++j) {
            DateTime test_date = base_date + DateTime::minutes(j);
            DateTime round_date = test_date.round(INTERVAL);
            EXPECT_EQ(round_date, base_date - INTERVAL);
        }
        for (int64_t j = -15; j < 15; ++j) {
            DateTime test_date = base_date + DateTime::minutes(j);
            DateTime round_date = test_date.round(INTERVAL);
            EXPECT_EQ(round_date, base_date);
        }
        for (int64_t j = 15; j < 30; ++j) {
            DateTime test_date = base_date + DateTime::minutes(j);
            DateTime round_date = test_date.round(INTERVAL);
            EXPECT_EQ(round_date, base_date + INTERVAL);
        }
    }
}

#include <random>

TEST(DateTime, round_edges) {
    const int NUMITEMS = 100;
    const DateTime INTERVAL = DateTime::minutes(30);
    const DateTime HALF_INTERVAL = DateTime::secs(INTERVAL.secs() / 2);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int64_t> dd(0, DateTime::NANOS_IN_HOUR);
    DateTime base_date;
    for (uint32_t day = 0; day < DateTime::MAXDATES; ++day) {
        // printf(".");
        base_date = base_date + DateTime::days(1);
        for (uint32_t hours = 0; hours < 24; ++hours) {
            DateTime base_time = base_date + DateTime::hours(hours);
            for (int j = 0; j < NUMITEMS; ++j) {
                int64_t nanos = dd(gen);
                DateTime time = base_time + DateTime::nsecs(nanos);
                DateTime round_time = time.round(INTERVAL);
                EXPECT_LE(round_time, time + HALF_INTERVAL);
                EXPECT_GE(round_time, time - HALF_INTERVAL);
            }
        }
    }
}

TEST(DateTime, advance) {
    DateTime now = DateTime::nsecs(100);
    DateTime interval = DateTime::nsecs(100);
    EXPECT_FALSE(now.advance(DateTime::nsecs(50), interval));
    EXPECT_FALSE(now.advance(DateTime::nsecs(99), interval));
    EXPECT_TRUE(now.advance(DateTime::nsecs(100), interval));
    EXPECT_EQ(now, DateTime::nsecs(200));
    EXPECT_FALSE(now.advance(DateTime::nsecs(150), interval));
    EXPECT_FALSE(now.advance(DateTime::nsecs(199), interval));
    EXPECT_TRUE(now.advance(DateTime::nsecs(200), interval));
    EXPECT_EQ(now, DateTime::nsecs(300));
    EXPECT_TRUE(now.advance(DateTime::nsecs(350), interval));
    EXPECT_EQ(now, DateTime::nsecs(400));
    EXPECT_TRUE(now.advance(DateTime::nsecs(500), interval));
    EXPECT_EQ(now, DateTime::nsecs(600));
}

TEST(DateTime, advance_templated) {
    DateTime now = DateTime::nsecs(100);
    constexpr auto INTERVAL = DateTime::nsecs(100).nsecs();
    EXPECT_FALSE(now.advance<INTERVAL>(DateTime::nsecs(50)));
    EXPECT_FALSE(now.advance<INTERVAL>(DateTime::nsecs(99)));
    EXPECT_TRUE(now.advance<INTERVAL>(DateTime::nsecs(100)));
    EXPECT_EQ(now, DateTime::nsecs(200));
    EXPECT_FALSE(now.advance<INTERVAL>(DateTime::nsecs(150)));
    EXPECT_FALSE(now.advance<INTERVAL>(DateTime::nsecs(199)));
    EXPECT_TRUE(now.advance<INTERVAL>(DateTime::nsecs(200)));
    EXPECT_EQ(now, DateTime::nsecs(300));
    EXPECT_TRUE(now.advance<INTERVAL>(DateTime::nsecs(350)));
    EXPECT_EQ(now, DateTime::nsecs(400));
    EXPECT_TRUE(now.advance<INTERVAL>(DateTime::nsecs(500)));
    EXPECT_EQ(now, DateTime::nsecs(600));
}

// Regression test for DateTime::round() bug fix
// Bug: returned rem instead of (ns - rem)
TEST(DateTime, RoundBugRegression) {
    // Test case that would fail with old bug
    DateTime time = DateTime::nsecs(1500);  // 1500 nanoseconds
    DateTime interval = DateTime::nsecs(1000);  // Round to 1000ns

    DateTime rounded = time.round(interval);

    // With bug: would return 500 (the remainder)
    // Fixed: should return 2000 (rounded up to nearest 1000)
    EXPECT_EQ(rounded.nsecs(), 2000);

    // Test another case
    time = DateTime::nsecs(1400);
    rounded = time.round(interval);
    // Should round down to 1000
    EXPECT_EQ(rounded.nsecs(), 1000);

    // Test with larger values
    time = DateTime::msecs(1250);  // 1.25 seconds
    interval = DateTime::secs(1);   // Round to 1 second
    rounded = time.round(interval);
    // Should round to 1 second (1000ms)
    EXPECT_EQ(rounded.msecs(), 1000);
}

// Test the power-of-2 optimization added later
TEST(DateTime, RoundPowerOfTwo) {
    // Test power-of-2 intervals (uses fast bit-masking path)
    std::vector<int64_t> power_of_2_intervals = {
        1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024,
        1000, 1000000, 1000000000  // 1μs, 1ms, 1s (common HFT intervals)
    };

    for (int64_t intns : power_of_2_intervals) {
        // Skip if not power of 2
        if ((intns & (intns - 1)) != 0) continue;

        DateTime interval = DateTime::nsecs(intns);
        DateTime time = DateTime::nsecs(intns + intns / 4);  // 1.25 * interval

        DateTime rounded = time.round(interval);

        // Should round to nearest interval
        int64_t expected = intns;  // Rounds down (0.25 < 0.5)
        EXPECT_EQ(rounded.nsecs(), expected);
    }
}