#pragma once

#include <cstdint>
#include <array>

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
    Histogram(double min_, double max_) {
        minimum = min_;
        maximum = max_;
    }
    void add(double value) {
        long kbin = lrint(((value - minimum) / (maximum - minimum)) * N);
        if (kbin < 0) kbin = 0;
        if (kbin >= (long)bins.size()) kbin = bins.size() - 1;
        Bin& bin(bins[kbin]);
        bin.sum += value;
        bin.sum2 += value * value;
        bin.count += 1;
    }
    Stats summary() {
        double sum = 0;
        uint64_t count = 0;
        for (const Bin& bin : bins) {
            sum += bin.sum;
            count += bin.count;
        }
        if (count == 0) {
            return {};
        }
        uint64_t totalcount = count;
        double totalsum = sum;
        Stats stats;
        stats.samples = totalcount;
        stats.average = totalsum / totalcount;
        sum = 0;
        count = 0;
        for (size_t j = 0; j < bins.size(); ++j) {
            const Bin& bin(bins[j]);
            count += bin.count;
            if (count >= totalcount / 2) {
                stats.median = bin.sum / bin.count;
                break;
            }
        }
        return stats;
    }
};

}  // namespace hbthreads