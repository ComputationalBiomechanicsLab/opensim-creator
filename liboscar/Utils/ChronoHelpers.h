#pragma once

#include <chrono>
#include <cmath>
#include <string>
#include <type_traits>

namespace osc
{
    // returns the linear interpolation between `a` and `b`, if the parameter `t` is inside [0, 1) (the linear
    // extrapolation otherwise)
    template<typename Rep, typename Period, typename Arithmetic>
    requires std::is_arithmetic_v<Rep> and std::is_arithmetic_v<Arithmetic>
    constexpr std::chrono::duration<Rep, Period> lerp(
        const std::chrono::duration<Rep, Period>& a,
        const std::chrono::duration<Rep, Period>& b,
        const Arithmetic& t)
    {
        return std::chrono::duration<Rep, Period>(static_cast<Rep>(std::lerp(a.count(), b.count(), t)));
    }

    // returns the linear interpolation between `a` and `b`, if the parameter `t` is inside [0, 1) (the linear
    // extrapolation otherwise)
    template<typename Clock, typename Duration, typename Arithmetic>
    requires std::is_arithmetic_v<Arithmetic>
    constexpr std::chrono::time_point<Clock, Duration> lerp(
        const std::chrono::time_point<Clock, Duration>& a,
        const std::chrono::time_point<Clock, Duration>& b,
        const Arithmetic& t)
    {
        return std::chrono::time_point<Clock, Duration>{lerp(a.time_since_epoch(), b.time_since_epoch(), t)};
    }

    // Returns the given time point as an ISO 8601 extended-format timestamp
    // (e.g. 2025-06-19 10:06:35+01:00) string.
    std::string to_iso8601_timestamp(std::chrono::zoned_seconds);
}
