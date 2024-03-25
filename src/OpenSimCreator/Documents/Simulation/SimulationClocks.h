#pragma once

#include <OpenSimCreator/Documents/Simulation/SimulationClock.h>

#include <oscar/Maths/ClosedInterval.h>
#include <oscar/Maths/Normalized.h>
#include <oscar/Utils/Algorithms.h>

namespace osc
{
    // clocks/progress associated with a single simulation
    class SimulationClocks final {
    public:
        constexpr SimulationClocks() = default;

        constexpr SimulationClocks(
            ClosedInterval<SimulationClock::time_point> timeRange_,
            Normalized<float> progress_ = 1.0f) :

            m_TimeRange{timeRange_},
            m_Progress{progress_}
        {}

        constexpr SimulationClock::time_point start() const { return m_TimeRange.lower; }
        constexpr SimulationClock::time_point current() const
        {
            return lerp(start(), end(), m_Progress.get());
        }
        constexpr SimulationClock::time_point end() const { return m_TimeRange.upper; }
        constexpr float progress() const { return m_Progress; }
    private:
        ClosedInterval<SimulationClock::time_point> m_TimeRange{SimulationClock::start(), SimulationClock::start()};
        Normalized<float> m_Progress = 1.0f;
    };
}
