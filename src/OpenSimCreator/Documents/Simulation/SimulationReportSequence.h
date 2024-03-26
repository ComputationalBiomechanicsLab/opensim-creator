#pragma once

#include <OpenSimCreator/Documents/Simulation/AuxiliaryValue.h>
#include <OpenSimCreator/Documents/Simulation/SimulationClock.h>

#include <oscar/Utils/CopyOnUpdPtr.h>

#include <span>

namespace OpenSim { class Model; }
namespace osc { class SimulationReportSequenceCursor; }
namespace SimTK { class State; }

namespace osc
{
    // represents an indexed sequence container that can be used in conjunction with
    // a `SimulationReportSequenceCursor` to produce realized states from a model
    class SimulationReportSequence final {
    public:
        SimulationReportSequence();

        size_t size() const;
        [[nodiscard]] bool empty() const { return size() == 0; }
        SimulationClock::time_point frontTime() const;
        SimulationClock::time_point backTime() const;
        std::optional<SimulationClock::time_point> prevTime(SimulationClock::time_point) const;
        std::optional<SimulationClock::time_point> nextTime(SimulationClock::time_point) const;
        std::optional<size_t> indexOfStateAfterOrIncluding(SimulationClock::time_point) const;

        void reserve(size_t);
        void emplace_back(SimTK::State&&, std::span<AuxiliaryValue const> auxiliaryValues = {});
        void seek(SimulationReportSequenceCursor&, OpenSim::Model const&, size_t i) const;
        SimulationClock::time_point timeOf(size_t i) const;

    private:
        class Impl;
        CopyOnUpdPtr<Impl> m_Impl;
    };
}
