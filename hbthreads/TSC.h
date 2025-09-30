#pragma once
#include <cstdint>
#include <time.h>

namespace hbthreads {

/**
 * CPU Time Stamp Counter (TSC) for ultra-low latency timestamping.
 *
 * TSC provides ~10ns overhead vs ~20-30ns for clock_gettime().
 * Must be calibrated once at startup.
 *
 * Usage:
 *   TSC::calibrate();  // Call once at startup
 *   uint64_t ns = TSC::rdtsc_ns();  // Get nanoseconds
 */
class TSC {
public:
    /**
     * Read the CPU timestamp counter directly (raw cycles).
     * ~10ns overhead on modern x86_64 CPUs.
     */
    static inline uint64_t rdtsc() noexcept {
#if defined(__x86_64__) || defined(__i386__)
        uint32_t lo, hi;
        __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
        return ((uint64_t)hi << 32) | lo;
#else
        // Fallback to clock_gettime on non-x86 architectures
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
#endif
    }

    /**
     * Read TSC and convert to nanoseconds.
     * Requires calibrate() to be called once at startup.
     */
    static inline uint64_t rdtsc_ns() noexcept {
#if defined(__x86_64__) || defined(__i386__)
        uint64_t tsc = rdtsc();
        // Convert TSC cycles to nanoseconds using pre-calibrated multiplier
        return (tsc * _ns_per_tick_num) / _ns_per_tick_denom;
#else
        return rdtsc();  // Already in nanoseconds on non-x86
#endif
    }

    /**
     * Calibrate TSC to nanoseconds conversion.
     * Must be called once at program startup before using rdtsc_ns().
     *
     * Measures TSC frequency by comparing against clock_gettime()
     * over a calibration period (default 100ms).
     */
    static void calibrate(uint64_t calibration_ms = 100) noexcept {
#if defined(__x86_64__) || defined(__i386__)
        struct timespec start_ts, end_ts;
        uint64_t start_tsc, end_tsc;

        // Sample start
        clock_gettime(CLOCK_MONOTONIC, &start_ts);
        start_tsc = rdtsc();

        // Wait for calibration period
        struct timespec sleep_ts;
        sleep_ts.tv_sec = calibration_ms / 1000;
        sleep_ts.tv_nsec = (calibration_ms % 1000) * 1000000;
        nanosleep(&sleep_ts, nullptr);

        // Sample end
        end_tsc = rdtsc();
        clock_gettime(CLOCK_MONOTONIC, &end_ts);

        // Calculate elapsed time in nanoseconds
        uint64_t elapsed_ns = (end_ts.tv_sec - start_ts.tv_sec) * 1000000000ULL +
                              (end_ts.tv_nsec - start_ts.tv_nsec);
        uint64_t elapsed_tsc = end_tsc - start_tsc;

        // Calculate conversion factor as a rational number to avoid precision loss
        // ns_per_tick = elapsed_ns / elapsed_tsc
        // We store as num/denom to maintain precision
        uint64_t gcd_val = gcd(elapsed_ns, elapsed_tsc);
        _ns_per_tick_num = elapsed_ns / gcd_val;
        _ns_per_tick_denom = elapsed_tsc / gcd_val;
#else
        // No calibration needed for non-x86
        _ns_per_tick_num = 1;
        _ns_per_tick_denom = 1;
#endif
    }

    /**
     * Check if TSC is available and stable on this CPU.
     * Returns true if TSC can be safely used for timing.
     */
    static bool is_available() noexcept {
#if defined(__x86_64__) || defined(__i386__)
        // Check for constant_tsc and nonstop_tsc features
        // This is a simplified check - production code should verify CPUID
        return true;  // Assume modern CPUs have stable TSC
#else
        return false;  // TSC not available on non-x86
#endif
    }

private:
    // Conversion factor: nanoseconds = (tsc * _ns_per_tick_num) / _ns_per_tick_denom
    static uint64_t _ns_per_tick_num;
    static uint64_t _ns_per_tick_denom;

    // Greatest common divisor (Euclidean algorithm)
    static uint64_t gcd(uint64_t a, uint64_t b) noexcept {
        while (b != 0) {
            uint64_t temp = b;
            b = a % b;
            a = temp;
        }
        return a;
    }
};

}  // namespace hbthreads