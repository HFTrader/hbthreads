#pragma once

#include <cstdint>
#include <cmath>
#include <limits>
#include <array>
#include <algorithm>

namespace hbthreads {

/**
 * Statistics results
 */
struct Stats {
    uint64_t samples;
    double median;
    double average;
};

/**
 * A simple histogram class with fixed N bins.
 */
template <size_t N>
struct Histogram {
    /**
     * The histogram bin containing aggregate stats
     */
    struct Bin {
        double sum = 0;
        double sum2 = 0;
        uint32_t count = 0;
    };
    std::array<Bin, N> bins;
    double minimum;
    double maximum;
    double minvalue;
    double maxvalue;

    Histogram(double min_, double max_) {
        minimum = min_;
        maximum = max_;
        reset();
    }

    void reset() {
        minvalue = std::numeric_limits<double>::max();
        maxvalue = std::numeric_limits<double>::min();
        for (auto& bin : bins) {
            bin.sum = bin.sum2 = 0;
            bin.count = 0;
        }
    }

    void add(double value) {
        if (value < minvalue) minvalue = value;
        if (value > maxvalue) maxvalue = value;

        // Handle edge case where minimum == maximum
        long kbin = 0;
        if (maximum > minimum) {
            double normalized = (value - minimum) / (maximum - minimum);
            kbin = long(normalized * N);
            // Clamp to valid range to prevent overflow
            kbin = std::max(0L, std::min(kbin, long(bins.size()) - 1));
        }

        Bin& bin(bins[kbin]);
        bin.sum += value;
        bin.sum2 += value * value;
        bin.count += 1;
    }

    uint64_t sum() const {
        double sum = 0;
        for (const Bin& bin : bins) {
            sum += bin.sum;
        }
        return sum;
    }

    uint64_t count() const {
        uint64_t count = 0;
        for (const Bin& bin : bins) {
            count += bin.count;
        }
        return count;
    }

    double percentile(double pct) {
        uint64_t totalcount = count();
        uint64_t counter = 0;
        double target = (pct / 100) * totalcount;
        for (size_t j = 0; j < bins.size(); ++j) {
            const Bin& bin(bins[j]);
            counter += bin.count;
            if (counter >= target) {
                return bin.sum / bin.count;
            }
        }
        return std::numeric_limits<double>::quiet_NaN();
    }
    Stats summary() {
        uint64_t n = count();
        if (n == 0) return {};
        Stats stats;
        stats.samples = n;
        stats.average = sum() / n;
        stats.median = percentile(50);
        return stats;
    }
};

}  // namespace hbthreads