#include <gtest/gtest.h>
#include "Histogram.h"
#include <limits>

using namespace hbthreads;

TEST(Histogram, Constructor) {
    Histogram<100> hist(0.0, 100.0);
    EXPECT_EQ(hist.count(), 0);
}

TEST(Histogram, BasicAdd) {
    Histogram<10> hist(0.0, 10.0);

    hist.add(5.0);
    EXPECT_EQ(hist.count(), 1);

    hist.add(7.5);
    EXPECT_EQ(hist.count(), 2);
}

TEST(Histogram, MinMax) {
    Histogram<10> hist(0.0, 100.0);

    hist.add(25.0);
    EXPECT_EQ(hist.minvalue, 25.0);
    EXPECT_EQ(hist.maxvalue, 25.0);

    hist.add(75.0);
    EXPECT_EQ(hist.minvalue, 25.0);
    EXPECT_EQ(hist.maxvalue, 75.0);

    hist.add(10.0);
    EXPECT_EQ(hist.minvalue, 10.0);
    EXPECT_EQ(hist.maxvalue, 75.0);
}

TEST(Histogram, Percentile) {
    Histogram<100> hist(0.0, 100.0);

    for (int i = 0; i < 100; i++) {
        hist.add(i);
    }

    EXPECT_EQ(hist.count(), 100);

    // Test approximate median
    double median = hist.percentile(50);
    EXPECT_NEAR(median, 50.0, 5.0);
}

TEST(Histogram, Reset) {
    Histogram<10> hist(0.0, 100.0);

    hist.add(50.0);
    hist.add(75.0);
    EXPECT_EQ(hist.count(), 2);

    hist.reset();
    EXPECT_EQ(hist.count(), 0);
}

// Regression test for division by zero bug fix
// Bug: minimum == maximum caused division by zero
TEST(Histogram, DivisionByZeroRegression) {
    // Create histogram with same min and max
    Histogram<10> hist(50.0, 50.0);

    // This would crash before fix
    hist.add(50.0);
    EXPECT_EQ(hist.count(), 1);

    // All values should go to bin 0
    hist.add(50.0);
    hist.add(50.0);
    EXPECT_EQ(hist.count(), 3);

    // Should not crash when getting summary
    Stats stats = hist.summary();
    EXPECT_EQ(stats.samples, 3);
}

// Regression test for overflow bug fix
// Bug: extreme values could overflow bin calculation
TEST(Histogram, OverflowRegression) {
    Histogram<100> hist(0.0, 100.0);

    // Test extreme negative value
    hist.add(-1000.0);
    EXPECT_EQ(hist.count(), 1);

    // Test extreme positive value
    hist.add(1000.0);
    EXPECT_EQ(hist.count(), 2);

    // Values outside range should still be tracked
    EXPECT_EQ(hist.minvalue, -1000.0);
    EXPECT_EQ(hist.maxvalue, 1000.0);
}

TEST(Histogram, BoundaryValues) {
    Histogram<10> hist(0.0, 100.0);

    // Test exact boundaries
    hist.add(0.0);
    hist.add(100.0);
    EXPECT_EQ(hist.count(), 2);

    // Test just outside boundaries
    hist.add(-0.1);
    hist.add(100.1);
    EXPECT_EQ(hist.count(), 4);
}

TEST(Histogram, ClampingBehavior) {
    Histogram<10> hist(0.0, 100.0);

    // Add many values outside range
    for (int i = -100; i < 0; i++) {
        hist.add(i);
    }
    for (int i = 101; i < 200; i++) {
        hist.add(i);
    }

    // Should not crash and should track all values
    EXPECT_EQ(hist.count(), 199);
}

TEST(Histogram, Summary) {
    Histogram<100> hist(0.0, 100.0);

    // Empty histogram
    Stats empty_stats = hist.summary();
    EXPECT_EQ(empty_stats.samples, 0);

    // Add values
    for (int i = 0; i < 10; i++) {
        hist.add(i * 10.0);
    }

    Stats stats = hist.summary();
    EXPECT_EQ(stats.samples, 10);
    EXPECT_NEAR(stats.average, 45.0, 1.0);
}