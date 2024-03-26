#pragma once

#include <OpenSimCreator/Documents/Simulation/ISimulationState.h>

namespace osc
{
    class BasicStateRef : public ISimulationState {
    public:
        explicit BasicStateRef(SimTK::State const& state_) :
            m_State{&state_}
        {}
    private:
        SimTK::State const& implGetState() const { return *m_State; }

        SimTK::State const* m_State;
    };
}
