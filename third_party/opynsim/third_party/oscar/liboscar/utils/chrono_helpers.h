#pragma once

#include <chrono>
#include <cmath>
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
}
