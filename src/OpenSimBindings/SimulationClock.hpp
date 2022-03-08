#pragma once

#include <chrono>
#include <ratio>

namespace osc
{
    // how time is represented by OpenSim/SimTK (i.e. seconds held in a `double`)
    struct SimulationClock {
        using rep = double;
        using period = std::ratio<1>;
        using duration = std::chrono::duration<rep, period>;
        using time_point = std::chrono::time_point<SimulationClock>;

        static time_point start() noexcept
        {
            return time_point(duration{0.0});
        }
    };
}
