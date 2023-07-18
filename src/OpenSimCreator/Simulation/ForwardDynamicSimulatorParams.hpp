#pragma once

#include "OpenSimCreator/Simulation/IntegratorMethod.hpp"
#include "OpenSimCreator/Simulation/SimulationClock.hpp"
#include "OpenSimCreator/Utils/ParamBlock.hpp"

namespace osc
{
    // simulation parameters
    struct ForwardDynamicSimulatorParams final {

        ForwardDynamicSimulatorParams();

        // final time for the simulation
        SimulationClock::time_point finalTime;

        // which integration method to use for the simulation
        IntegratorMethod integratorMethodUsed;

        // the time interval, in simulation time, between report updates
        SimulationClock::duration reportingInterval;

        // max number of *internal* steps that may be taken within a single call
        // to the integrator's stepTo or stepBy function
        //
        // this is mostly an internal concern, but can affect how regularly the
        // simulator reports updates (e.g. a lower number here *may* mean more
        // frequent per-significant-step updates)
        int integratorStepLimit;

        // minimum step, in time, that the integrator should attempt
        //
        // some integrators just ignore this
        SimulationClock::duration integratorMinimumStepSize;

        // maximum step, in time, that an integrator can attempt
        //
        // e.g. even if the integrator *thinks* it can skip 10s of simulation time
        // it still *must* integrate to this size and return to the caller (i.e. the
        // simulator) to report the state at this maximum time
        SimulationClock::duration integratorMaximumStepSize;

        // accuracy of the integrator
        //
        // this only does something if the integrator is error-controlled and able
        // to improve accuracy (e.g. by taking many more steps)
        double integratorAccuracy;
    };

    bool operator==(ForwardDynamicSimulatorParams const& a, ForwardDynamicSimulatorParams const& b);

    // convert to a generic parameter block (for UI binding)
    ParamBlock ToParamBlock(ForwardDynamicSimulatorParams const&);
    ForwardDynamicSimulatorParams FromParamBlock(ParamBlock const&);
}
