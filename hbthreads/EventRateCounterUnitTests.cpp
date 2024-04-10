#include <gtest/gtest.h>
#include "EventRateCounter.h"

using namespace hbthreads;

TEST(EventRateCounter, Basic) {
    for (int msecs = 1; msecs <= 200; ++msecs) {
        EventRateCounter counter(DateTime::secs(1), DateTime::msecs(msecs));
        DateTime now = DateTime::now();
        counter.advance(now);
        counter.add(1);
        EXPECT_EQ(counter, 1);
        counter.add(1);
        EXPECT_EQ(counter, 2);
        now = now + DateTime::secs(1);
        counter.advance(now);
        EXPECT_EQ(counter, 0);
        for (int j = 0; j < 1000; ++j) {
            counter.add(1);
            // std::cout << msecs << "," << j << "," << counter << std::endl;
            EXPECT_LE((int)counter, j + 1);
            EXPECT_GE((int)counter, j + 1 - 2 * msecs);
            counter.advance(now + DateTime::msecs(j));
        }
    }
}

TEST(EventRateCounter, Cycle) {
    EventRateCounter counter(DateTime::secs(1), DateTime::msecs(1));
    DateTime now = DateTime::now();
    counter.advance(now);
    counter.add(1);
    EXPECT_EQ(counter, 1);
    counter.add(1);
    EXPECT_EQ(counter, 2);
    now = now + DateTime::secs(1);
    counter.advance(now);
    EXPECT_EQ(counter, 0);
}
