#include <OpenSimCreator/Documents/Simulation/ForwardDynamicSimulation.h>

#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSimCreator/Documents/Model/BasicModelStatePair.h>
#include <OpenSimCreator/Documents/Simulation/ForwardDynamicSimulatorParams.h>
#include <gtest/gtest.h>

#include <chrono>

using namespace osc;
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

TEST(ForwardDynamicSimulation, CanChangeEndTime)
{
    BasicModelStatePair modelState;
    ForwardDynamicSimulatorParams params;
    params.finalTime = SimulationClock::start();

    ForwardDynamicSimulation sim{modelState, params};
    ASSERT_TRUE(sim.canChangeEndTime());
}

TEST(ForwardDynamicSimulation, IncreasingTheEndTimeWorksAsExpected)
{
    using namespace std::literals;

    BasicModelStatePair modelState;

    // set up the simulation to produce two reports (start, end)
    ForwardDynamicSimulatorParams params;
    params.finalTime = SimulationClock::start() + 1s;
    params.reportingInterval = 1s;

    // run the simulation and wait for it to complete
    ForwardDynamicSimulation sim{modelState, params};
    sim.join();
    ASSERT_EQ(sim.getStatus(), SimulationStatus::Completed);

    // ensure it has completed and has the expected number of reports
    {
        auto const reports = sim.getAllSimulationReports();
        ASSERT_EQ(reports.size(), 2);
        ASSERT_EQ(reports.at(0).getTime(), SimulationClock::start());
        ASSERT_EQ(reports.at(1).getTime(), SimulationClock::start() + 1s);
    }

    // then increase the end time and wait for the new simulation to complete
    sim.requestNewEndTime(SimulationClock::start() + 2s);
    sim.join();
    ASSERT_EQ(sim.getStatus(), SimulationStatus::Completed);


    // ensure the extended simulation is as-expected
    {
        auto const reports = sim.getAllSimulationReports();
        ASSERT_EQ(reports.size(), 3);
        ASSERT_EQ(reports.at(0).getTime(), SimulationClock::start());
        ASSERT_EQ(reports.at(1).getTime(), SimulationClock::start() + 1s);
        ASSERT_EQ(reports.at(2).getTime(), SimulationClock::start() + 2s);
    }
}

TEST(ForwardDynamicSimulation, DecreasingEndTimeWorksAsExpected)
{
    using namespace std::literals;

    BasicModelStatePair modelState;

    // set up the simulation to produce two reports (start, end)
    ForwardDynamicSimulatorParams params;
    params.finalTime = SimulationClock::start() + 2s;
    params.reportingInterval = 1s;

    // run the simulation and wait for it to complete
    ForwardDynamicSimulation sim{modelState, params};
    sim.join();
    ASSERT_EQ(sim.getStatus(), SimulationStatus::Completed);

    // ensure it has completed and has the expected number of reports
    {
        auto const reports = sim.getAllSimulationReports();
        ASSERT_EQ(reports.size(), 3);
        ASSERT_EQ(reports.at(0).getTime(), SimulationClock::start());
        ASSERT_EQ(reports.at(1).getTime(), SimulationClock::start() + 1s);
        ASSERT_EQ(reports.at(2).getTime(), SimulationClock::start() + 2s);
    }

    // then decrease the end time, which shouldn't require waiting
    sim.requestNewEndTime(SimulationClock::start() + 1s);
    ASSERT_EQ(sim.getStatus(), SimulationStatus::Completed);  // should be immediately true: in-memory truncation
    ASSERT_EQ(sim.getEndTime(), SimulationClock::start() + 1s);


    // ensure the shrunk simulation is as-expected
    {
        auto const reports = sim.getAllSimulationReports();
        ASSERT_EQ(reports.size(), 2);
        ASSERT_EQ(reports.at(0).getTime(), SimulationClock::start());
        ASSERT_EQ(reports.at(1).getTime(), SimulationClock::start() + 1s);
    }
}
