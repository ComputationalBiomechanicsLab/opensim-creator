#pragma once

#include <chrono>
#include <ratio>

namespace osc
{
    // definition of application's runtime clock
    struct AppClock final {
        using rep = float;
        using period = std::ratio<1>;
        using duration = std::chrono::duration<rep, period>;
        using time_point = std::chrono::time_point<AppClock>;
    };
    using AppNanos = std::chrono::duration<AppClock::rep, std::nano>;
    using AppMicros = std::chrono::duration<AppClock::rep, std::micro>;
    using AppMillis = std::chrono::duration<AppClock::rep, std::milli>;
    using AppSeconds = std::chrono::duration<AppClock::rep>;
    using AppMinutes = std::chrono::duration<AppClock::rep, std::ratio<60>>;
    using AppHours = std::chrono::duration<AppClock::rep, std::ratio<3600>>;
}