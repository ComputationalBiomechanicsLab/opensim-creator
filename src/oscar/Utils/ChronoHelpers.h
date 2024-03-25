#pragma once

#include <chrono>
#include <numeric>
#include <type_traits>

namespace osc
{
    template<typename Rep, typename Period, typename Arithmetic>
    constexpr std::chrono::duration<Rep, Period> lerp(
        std::chrono::duration<Rep, Period> const& a,
        std::chrono::duration<Rep, Period> const& b,
        Arithmetic const& t)

        requires std::is_arithmetic_v<Rep> and std::is_arithmetic_v<Arithmetic>
    {
        return std::chrono::duration<Rep, Period>(static_cast<Rep>(std::lerp(a.count(), b.count(), t)));
    }

    template<typename Clock, typename Duration, typename Arithmetic>
    constexpr std::chrono::time_point<Clock, Duration> lerp(
        std::chrono::time_point<Clock, Duration> const& a,
        std::chrono::time_point<Clock, Duration> const& b,
        Arithmetic const& t)

        requires std::is_arithmetic_v<Arithmetic>
    {
        return std::chrono::time_point<Clock, Duration>{lerp(a.time_since_epoch(), b.time_since_epoch(), t)};
    }
}
