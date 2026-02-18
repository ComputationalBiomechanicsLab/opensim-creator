#pragma once

#include <chrono>
#include <ratio>

namespace osc
{
    // a `std::chrono`-compatible representation of how time is represented in
    // OpenSim/SimTK (i.e. seconds held in a `double`)
    struct SimulationClock final {
        using rep = double;
        using period = std::ratio<1>;
        using duration = std::chrono::duration<rep, period>;
        using time_point = std::chrono::time_point<SimulationClock>;

        static time_point start()
        {
            return time_point(duration{0.0});
        }
    };
}
