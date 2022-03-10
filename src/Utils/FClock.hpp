#pragma once

#include <chrono>
#include <ratio>

namespace osc
{
    struct FClock {
        using rep = float;
        using period = std::ratio<1>;
        using duration = std::chrono::duration<rep, period>;
        using time_point = std::chrono::time_point<FClock>;
    };

    using FNanos = std::chrono::duration<FClock::rep, std::nano>;
    using FMicros = std::chrono::duration<FClock::rep, std::micro>;
    using FMillis = std::chrono::duration<FClock::rep, std::milli>;
    using FSeconds = std::chrono::duration<FClock::rep>;
    using FMinutes = std::chrono::duration<FClock::rep, std::ratio<60>>;
    using FHours = std::chrono::duration<FClock::rep, std::ratio<3600>>;
}
