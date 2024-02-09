#include <OpenSimCreator/Documents/Simulation/ForwardDynamicSimulation.h>

#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSimCreator/Documents/Model/BasicModelStatePair.h>
#include <OpenSimCreator/Documents/Simulation/ForwardDynamicSimulatorParams.h>
#include <gtest/gtest.h>

using osc::BasicModelStatePair;
using osc::ForwardDynamicSimulation;
using osc::ForwardDynamicSimulatorParams;
using osc::FromParamBlock;
using osc::SimulationClock;

TEST(ForwardDynamicSimulation, CanInitFromBasicModel)
{
    BasicModelStatePair modelState;

    ForwardDynamicSimulatorParams params;
    params.finalTime = SimulationClock::start();  // don't run a full sim

    ForwardDynamicSimulation sim{modelState, params};

    // just ensure calling these doesn't throw: actually testing these would
    // require running the simulation to completion because launching a simulation
    // is launching a background worker with a non-deterministic speed etc.

    sim.getModel()->getSystem();
    sim.getModel()->getWorkingState();
    sim.getNumReports();
    sim.getAllSimulationReports();
    sim.getStatus();
    sim.getCurTime();
    ASSERT_EQ(sim.getStartTime(), SimulationClock::start());
    ASSERT_EQ(sim.getEndTime(), params.finalTime);
    sim.getProgress();
    ASSERT_EQ(FromParamBlock(sim.getParams()), params);
    ASSERT_FALSE(sim.getOutputExtractors().empty());
    sim.requestStop();
    sim.stop();
    ASSERT_EQ(modelState.getFixupScaleFactor(), sim.getFixupScaleFactor());
    float newSf = sim.getFixupScaleFactor() + 1.0f;
    sim.setFixupScaleFactor(newSf);
    ASSERT_EQ(sim.getFixupScaleFactor(), newSf);
}
