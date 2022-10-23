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