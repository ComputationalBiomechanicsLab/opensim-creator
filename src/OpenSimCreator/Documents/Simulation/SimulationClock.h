#pragma once

#include <oscar/Maths/CommonFunctions.h>

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

    // returns a `Simulation::time_point` that is lerped between `a` and `b` by `t`
    template<typename Arithmetic>
    constexpr SimulationClock::time_point lerp(SimulationClock::time_point a, SimulationClock::time_point b, Arithmetic const& t)
        requires std::is_arithmetic_v<Arithmetic>
    {
        auto tEpoch = lerp(a.time_since_epoch(), b.time_since_epoch(), t);
        return SimulationClock::time_point(tEpoch);
    }
}
