#include <OpenSimCreator/Simulation/ForwardDynamicSimulation.hpp>

#include <gtest/gtest.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSimCreator/Model/BasicModelStatePair.hpp>
#include <OpenSimCreator/Simulation/ForwardDynamicSimulatorParams.hpp>

TEST(ForwardDynamicSimulation, CanInitFromBasicModel)
{
    osc::BasicModelStatePair modelState;

    osc::ForwardDynamicSimulatorParams params;
    params.finalTime = osc::SimulationClock::start();  // don't run a full sim

    osc::ForwardDynamicSimulation sim{modelState, params};

    // just ensure calling these doesn't throw: actually testing these would
    // require running the simulation to completion because launching a simulation
    // is launching a background worker with a non-deterministic speed etc.

    sim.getModel()->getSystem();
    sim.getModel()->getWorkingState();
    sim.getNumReports();
    sim.getAllSimulationReports();
    sim.getStatus();
    sim.getCurTime();
    ASSERT_EQ(sim.getStartTime(), osc::SimulationClock::start());
    ASSERT_EQ(sim.getEndTime(), params.finalTime);
    sim.getProgress();
    ASSERT_EQ(osc::FromParamBlock(sim.getParams()), params);
    ASSERT_FALSE(sim.getOutputExtractors().empty());
    sim.requestStop();
    sim.stop();
    ASSERT_EQ(modelState.getFixupScaleFactor(), sim.getFixupScaleFactor());
    float newSf = sim.getFixupScaleFactor() + 1.0f;
    sim.setFixupScaleFactor(newSf);
    ASSERT_EQ(sim.getFixupScaleFactor(), newSf);
}
