#pragma once

#include "src/OpenSimBindings/IntegratorMethod.hpp"
#include "src/OpenSimBindings/ParamBlock.hpp"
#include "src/OpenSimBindings/SimulationClock.hpp"

namespace osc
{
    // simulation parameters
    struct ForwardDynamicSimulatorParams final {

        // final time for the simulation
        SimulationClock::time_point FinalTime;

        // which integration method to use for the simulation
        IntegratorMethod IntegratorMethodUsed;

        // the time interval, in simulation time, between report updates
        SimulationClock::duration ReportingInterval;

                                                                 // max number of *internal* steps that may be taken within a single call
                                                                 // to the integrator's stepTo or stepBy function
                                                                 //
                                                                 // this is mostly an internal concern, but can affect how regularly the
                                                                 // simulator reports updates (e.g. a lower number here *may* mean more
                                                                 // frequent per-significant-step updates)
        int IntegratorStepLimit;

        // minimum step, in time, that the integrator should attempt
        //
        // some integrators just ignore this
        SimulationClock::duration IntegratorMinimumStepSize;

        // maximum step, in time, that an integrator can attempt
        //
        // e.g. even if the integrator *thinks* it can skip 10s of simulation time
        // it still *must* integrate to this size and return to the caller (i.e. the
        // simulator) to report the state at this maximum time
        SimulationClock::duration IntegratorMaximumStepSize;

        // accuracy of the integrator
        //
        // this only does something if the integrator is error-controlled and able
        // to improve accuracy (e.g. by taking many more steps)
        double IntegratorAccuracy;

        ForwardDynamicSimulatorParams();
    };

    // convert to a generic parameter block (for UI binding)
    ParamBlock ToParamBlock(ForwardDynamicSimulatorParams const&);
    ForwardDynamicSimulatorParams FromParamBlock(ParamBlock const&);
}