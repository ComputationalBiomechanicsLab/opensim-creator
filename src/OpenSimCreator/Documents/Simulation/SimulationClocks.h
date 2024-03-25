#pragma once

#include <OpenSimCreator/Documents/Simulation/SimulationClock.h>

#include <oscar/Maths/ClosedInterval.h>
#include <oscar/Maths/Normalized.h>
#include <oscar/Utils/Algorithms.h>
#include <oscar/Utils/ChronoHelpers.h>

namespace osc
{
    // clocks/progress associated with a single simulation
    class SimulationClocks final {
    public:
        constexpr SimulationClocks() = default;

        constexpr SimulationClocks(
            SimulationClock::time_point singlePoint_,
            bool completed = true) :

            SimulationClocks{{singlePoint_, singlePoint_}, completed ? 1.0f : 0.0f}
        {}

        constexpr SimulationClocks(
            ClosedInterval<SimulationClock::time_point> timeRange_,
            Normalized<float> progress_ = 1.0f) :

            m_TimeRange{timeRange_},
            m_Progress{progress_}
        {}

        constexpr SimulationClocks(
            ClosedInterval<SimulationClock::time_point> timeRange_,
            SimulationClock::time_point current_) :

            m_TimeRange{timeRange_},
            m_Progress{static_cast<float>((current_ - m_TimeRange.lower)/(m_TimeRange.upper - m_TimeRange.lower))}
        {
            // TODO: provide invlerp (inverse LERP), rather than calculating the above
        }

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
