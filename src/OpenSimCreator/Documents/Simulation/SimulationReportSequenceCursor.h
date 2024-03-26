#pragma once

#include <OpenSimCreator/Documents/Simulation/AuxiliaryValue.h>
#include <OpenSimCreator/Documents/Simulation/ISimulationState.h>
#include <OpenSimCreator/Documents/Simulation/SimulationClock.h>

#include <oscar/Utils/CopyOnUpdPtr.h>
#include <oscar/Utils/UID.h>

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
    class SimulationReportSequenceCursor final : public ISimulationState {
    public:
        SimulationReportSequenceCursor();

        friend bool operator==(SimulationReportSequenceCursor const& lhs, SimulationReportSequenceCursor const& rhs)
        {
            return lhs.m_Impl == rhs.m_Impl;
        }
    private:
        SimTK::State const& implGetState() const override;
        std::optional<float> implFindAuxiliaryValue(UID) const override;

        friend class SimulationReportSequence;
        void setState(CopyOnUpdPtr<SimTK::State> const&);
        void setAuxiliaryVariables(std::span<AuxiliaryValue const>);

        class Impl;
        CopyOnUpdPtr<Impl> m_Impl;
    };
}
