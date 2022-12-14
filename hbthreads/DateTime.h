#pragma once
#include <cstdint>

//--------------------------------------------------
// DateTime to give semantics to time values
//--------------------------------------------------

namespace hbthreads {

//! A wrapper around a 64-bit time in nanoseconds
//! If a date, it can be epoch but it can also be an interval.
struct DateTime {
    //! Zero it
    DateTime() : epochns(0) {
    }
    //! Construct a Datetime from seconds
    static DateTime secs(int64_t s) {
        return DateTime(s * 1000000000LL);
    }
    //! Construct a Datetime from milliseconds
    static DateTime msecs(int64_t ms) {
        return DateTime(ms * 1000000LL);
    }
    //! Construct a Datetime from microseconds
    static DateTime usecs(int64_t us) {
        return DateTime(us * 1000LL);
    }
    //! Construct a Datetime from nanoseconds
    static DateTime nsecs(int64_t ns) {
        return DateTime(ns);
    }
    //! Returns the number of entire milliseconds
    std::int64_t msecs() const {
        return epochns / 1000000LL;
    }
    //! Returns the number of entire microseconds
    std::int64_t usecs() const {
        return epochns / 1000LL;
    }
    //! Returns the number of nanoseconds
    std::int64_t nsecs() const {
        return epochns;
    }
    //! Returns the number of seconds
    std::int64_t secs() const {
        return epochns / 1000000000LL;
    }

    //! \defgroup Operators DateTime operator overloading
    //! @{

    //! Binary subtraction
    inline DateTime operator-(DateTime rhs) const {
        return DateTime(epochns - rhs.epochns);
    }
    //! Binary addition
    inline DateTime operator+(DateTime rhs) const {
        return DateTime(epochns + rhs.epochns);
    }
    //! Unary negation
    inline DateTime operator-() const {
        return DateTime(-epochns);
    }
    //! Binary less
    inline bool operator<(DateTime rhs) const {
        return epochns < rhs.epochns;
    }
    //! Binary greater
    inline bool operator>(DateTime rhs) const {
        return epochns > rhs.epochns;
    }
    //! Binary less or equal
    inline bool operator<=(DateTime rhs) const {
        return epochns <= rhs.epochns;
    }
    //! Binary greater or equal
    inline bool operator>=(DateTime rhs) const {
        return epochns >= rhs.epochns;
    }
    //! Binary equal
    inline bool operator==(DateTime rhs) const {
        return epochns == rhs.epochns;
    }
    //! @}

    //! Returns a zero interval
    static DateTime zero() {
        return DateTime(0);
    }

    //! Returns the current time in epoch/UTC
    enum class ClockType : uint8_t { RealTime = 1, Monotonic = 2 };
    static DateTime now(ClockType clock = ClockType::RealTime);

private:
    //! Force user to use the static methods otherwise they will
    //! use it wrong, believe me
    explicit DateTime(int64_t ns) : epochns(ns) {
    }
    //! All this boilerplate for me? Awwww
    int64_t epochns;
};

}  // namespace hbthreads
