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

TEST(EventRateCounter, LargeJump) {
    EventRateCounter counter(DateTime::secs(1), DateTime::msecs(10));
    DateTime now = DateTime::now();
    counter.advance(now);
    counter.add(100);
    EXPECT_EQ(counter, 100);

    // Jump far into the future (beyond window size)
    now = now + DateTime::secs(10);
    counter.advance(now);
    EXPECT_EQ(counter, 0);
}

TEST(EventRateCounter, SlidingWindow) {
    EventRateCounter counter(DateTime::secs(1), DateTime::msecs(100));
    DateTime now = DateTime::now();
    counter.advance(now);

    // Add events over time
    for (int i = 0; i < 10; i++) {
        counter.add(1);
        now = now + DateTime::msecs(100);
        counter.advance(now);
    }

    // 10 events were added at t+0ms .. t+900ms and the clock now reads t+1000ms.
    // The ring holds exactly 1s/100ms = 10 slots, so advancing onto slot 10
    // reuses slot 0: the event added at t+0 is exactly one full expiration
    // interval old and has therefore expired. 9 remain, not 10.
    EXPECT_EQ(counter.count(), 9);

    // Advance another 100ms - the next oldest event drops off
    now = now + DateTime::msecs(100);
    counter.advance(now);
    EXPECT_EQ(counter.count(), 8);
}

TEST(EventRateCounter, MultipleAddsBeforeAdvance) {
    EventRateCounter counter(DateTime::secs(1), DateTime::msecs(10));
    DateTime now = DateTime::now();
    counter.advance(now);

    // Add multiple events in same slot
    counter.add(5);
    counter.add(3);
    counter.add(2);
    EXPECT_EQ(counter, 10);

    // Advance and verify total preserved
    now = now + DateTime::msecs(5);
    counter.advance(now);
    EXPECT_EQ(counter, 10);
}

TEST(EventRateCounter, ZeroEvents) {
    EventRateCounter counter(DateTime::secs(1), DateTime::msecs(10));
    DateTime now = DateTime::now();
    counter.advance(now);

    // Don't add any events
    EXPECT_EQ(counter, 0);

    // Advance time
    now = now + DateTime::msecs(500);
    counter.advance(now);
    EXPECT_EQ(counter, 0);
}

TEST(EventRateCounter, HighFrequency) {
    EventRateCounter counter(DateTime::msecs(100), DateTime::msecs(1));
    DateTime now = DateTime::now();
    counter.advance(now);

    // Add many events rapidly
    for (int i = 0; i < 100; i++) {
        counter.add(1);
        now = now + DateTime::msecs(1);
        counter.advance(now);
    }

    // Same boundary as SlidingWindow: 100 events at t+0ms .. t+99ms with the
    // clock at t+100ms. The ring is 100ms/1ms = 100 slots, so the event from
    // t+0 is exactly one expiration interval old and has expired.
    EXPECT_EQ(counter.count(), 99);

    // Advance past window
    now = now + DateTime::msecs(100);
    counter.advance(now);
    EXPECT_EQ(counter.count(), 0);
}

TEST(EventRateCounter, PartialExpiration) {
    EventRateCounter counter(DateTime::secs(1), DateTime::msecs(100));
    DateTime now = DateTime::now();
    counter.advance(now);

    // Add 10 events at start
    counter.add(10);
    EXPECT_EQ(counter, 10);

    // Advance 500ms and add 5 more
    now = now + DateTime::msecs(500);
    counter.advance(now);
    counter.add(5);
    EXPECT_EQ(counter, 15);

    // Advance another 600ms - first batch should expire
    now = now + DateTime::msecs(600);
    counter.advance(now);
    EXPECT_EQ(counter, 5);
}

TEST(EventRateCounter, SameTimeAdvance) {
    EventRateCounter counter(DateTime::secs(1), DateTime::msecs(10));
    DateTime now = DateTime::now();
    counter.advance(now);
    counter.add(5);

    // Advance to same time - should be no-op
    counter.advance(now);
    EXPECT_EQ(counter, 5);

    counter.add(3);
    EXPECT_EQ(counter, 8);
}
