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

        void emplace_back(SimTK::State const&, std::span<AuxiliaryValue const> auxiliaryValues);
        void seek(SimulationReportSequenceCursor&, OpenSim::Model const&, size_t i) const;

    private:
        class Impl;
        CopyOnUpdPtr<Impl> m_Impl;
    };
}
