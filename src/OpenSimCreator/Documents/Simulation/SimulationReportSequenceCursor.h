#pragma once

#include <OpenSimCreator/Documents/Simulation/AuxiliaryValue.h>
#include <OpenSimCreator/Documents/Simulation/SimulationClock.h>

#include <oscar/Utils/CopyOnUpdPtr.h>
#include <oscar/Utils/UID.h>

#include <cstddef>
#include <optional>

namespace SimTK { class State; }

namespace osc
{
    // a cursor to a single "report" within a `SimulationReportSequence`
    //
    // the idea behind is that you re-use this cursor when traversing over
    // `SimulationReportSequence`s, because this cursor _must_ hold onto one
    // state, but the sequence _may_ hold onto none (i.e. it can just materialize
    // the state on-demand when a cursor comes along)
    class SimulationReportSequenceCursor final {
    public:
        SimulationReportSequenceCursor();

        size_t index() const;
        SimulationClock::time_point time() const;
        SimTK::State const& state() const;
        std::optional<float> findAuxiliaryValue(UID) const;

    private:
        friend class SimulationReportSequence;

        void setIndex(size_t newIndex);
        SimTK::State& updState();
        void clearAuxiliaryValues();
        void setAuxiliaryValue(AuxiliaryValue);

        class Impl;
        CopyOnUpdPtr<Impl> m_Impl;
    };
}
