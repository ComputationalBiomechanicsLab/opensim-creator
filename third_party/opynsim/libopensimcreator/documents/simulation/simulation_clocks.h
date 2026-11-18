#pragma once

#include <libopensimcreator/documents/simulation/simulation_clock.h>

#include <liboscar/maths/closed_interval.h>
#include <liboscar/maths/normalized.h>
#include <liboscar/utilities/chrono_helpers.h>

namespace osc
{
    // clocks/progress associated with a single simulation
    class SimulationClocks final {
    public:
        constexpr SimulationClocks() = default;

        constexpr explicit SimulationClocks(
            SimulationClock::time_point single_point, bool completed = true) :

            SimulationClocks{{single_point, single_point}, completed ? 1.0f : 0.0f}
        {}

        constexpr explicit SimulationClocks(
            ClosedInterval<SimulationClock::time_point> time_range,
            Normalized<float> progress = 1.0f) :

            time_range_{time_range},
            progress_{progress}
        {}

        constexpr SimulationClocks(
            ClosedInterval<SimulationClock::time_point> time_range,
            SimulationClock::time_point current) :

            time_range_{time_range},
            progress_{static_cast<float>(
                (current - time_range_.lower) / (time_range_.upper - time_range_.lower))}
        {
            // TODO: provide invlerp (inverse LERP), rather than calculating the above
        }

        constexpr SimulationClock::time_point start() const { return time_range_.lower; }
        constexpr SimulationClock::time_point current() const
        {
            return lerp(start(), end(), progress_.get());
        }
        constexpr SimulationClock::time_point end() const { return time_range_.upper; }
        constexpr float progress() const { return progress_; }

    private:
        ClosedInterval<SimulationClock::time_point> time_range_{
            SimulationClock::start(), SimulationClock::start()};
        Normalized<float> progress_ = 1.0f;
    };
}
