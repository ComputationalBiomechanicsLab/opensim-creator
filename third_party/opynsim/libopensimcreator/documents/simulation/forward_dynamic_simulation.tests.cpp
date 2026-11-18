#include "forward_dynamic_simulation.h"

#include <libopensimcreator/documents/model/basic_model_state_pair_with_shared_environment.h>
#include <libopensimcreator/documents/simulation/forward_dynamic_simulator_params.h>

#include <gtest/gtest.h>
#include <OpenSim/Simulation/Model/Model.h>

#include <chrono>

using namespace osc;

TEST(ForwardDynamicSimulation, CanInitFromBasicModel)
{
    BasicModelStatePairWithSharedEnvironment model_state;

    ForwardDynamicSimulatorParams params;
    params.final_time = SimulationClock::start();  // don't run a full sim

    ForwardDynamicSimulation sim{model_state, params};

    // just ensure calling these doesn't throw: actually testing these would
    // require running the simulation to completion because launching a simulation
    // is launching a background worker with a non-deterministic speed etc.

    sim.get_model()->getSystem();
    sim.get_model()->getWorkingState();
    sim.get_num_reports();
    sim.get_all_simulation_reports();
    sim.get_status();
    sim.get_cur_time();
    ASSERT_EQ(sim.get_start_time(), SimulationClock::start());
    ASSERT_EQ(sim.get_end_time(), params.final_time);
    sim.get_progress();
    ASSERT_EQ(from_param_block(sim.get_params()), params);
    ASSERT_FALSE(sim.get_output_extractors().empty());
    sim.request_stop();
    sim.stop();
    ASSERT_EQ(model_state.get_fixup_scale_factor(), sim.get_fixup_scale_factor());
    float new_scale_factor = sim.get_fixup_scale_factor() + 1.0f;
    sim.set_fixup_scale_factor(new_scale_factor);
    ASSERT_EQ(sim.get_fixup_scale_factor(), new_scale_factor);
}

TEST(ForwardDynamicSimulation, CanChangeEndTime)
{
    BasicModelStatePairWithSharedEnvironment model_state;
    ForwardDynamicSimulatorParams params;
    params.final_time = SimulationClock::start();

    ForwardDynamicSimulation sim{model_state, params};
    ASSERT_TRUE(sim.can_change_end_time());
}

TEST(ForwardDynamicSimulation, IncreasingTheEndTimeWorksAsExpected)
{
    using namespace std::literals;

    BasicModelStatePairWithSharedEnvironment model_state;

    // set up the simulation to produce two reports (start, end)
    ForwardDynamicSimulatorParams params;
    params.final_time = SimulationClock::start() + 1s;
    params.reporting_interval = 1s;

    // run the simulation and wait for it to complete
    ForwardDynamicSimulation sim{model_state, params};
    sim.join();
    ASSERT_EQ(sim.get_status(), SimulationStatus::Completed);

    // ensure it has completed and has the expected number of reports
    {
        const auto reports = sim.get_all_simulation_reports();
        ASSERT_EQ(reports.size(), 2);
        ASSERT_EQ(reports.at(0).get_time(), SimulationClock::start());
        ASSERT_EQ(reports.at(1).get_time(), SimulationClock::start() + 1s);
    }

    // then increase the end time and wait for the new simulation to complete
    sim.request_new_end_time(SimulationClock::start() + 2s);
    sim.join();
    ASSERT_EQ(sim.get_status(), SimulationStatus::Completed);

    // ensure the extended simulation is as-expected
    {
        const auto reports = sim.get_all_simulation_reports();
        ASSERT_EQ(reports.size(), 3);
        ASSERT_EQ(reports.at(0).get_time(), SimulationClock::start());
        ASSERT_EQ(reports.at(1).get_time(), SimulationClock::start() + 1s);
        ASSERT_EQ(reports.at(2).get_time(), SimulationClock::start() + 2s);
    }
}

TEST(ForwardDynamicSimulation, DecreasingEndTimeWorksAsExpected)
{
    using namespace std::literals;

    BasicModelStatePairWithSharedEnvironment model_state;

    // set up the simulation to produce two reports (start, end)
    ForwardDynamicSimulatorParams params;
    params.final_time = SimulationClock::start() + 2s;
    params.reporting_interval = 1s;

    // run the simulation and wait for it to complete
    ForwardDynamicSimulation sim{model_state, params};
    sim.join();
    ASSERT_EQ(sim.get_status(), SimulationStatus::Completed);

    // ensure it has completed and has the expected number of reports
    {
        const auto reports = sim.get_all_simulation_reports();
        ASSERT_EQ(reports.size(), 3);
        ASSERT_EQ(reports.at(0).get_time(), SimulationClock::start());
        ASSERT_EQ(reports.at(1).get_time(), SimulationClock::start() + 1s);
        ASSERT_EQ(reports.at(2).get_time(), SimulationClock::start() + 2s);
    }

    // then decrease the end time, which shouldn't require waiting
    sim.request_new_end_time(SimulationClock::start() + 1s);
    ASSERT_EQ(sim.get_status(), SimulationStatus::Completed)
        << "this should be immediately true: in-memory truncation";
    ASSERT_EQ(sim.get_end_time(), SimulationClock::start() + 1s);

    // ensure the shrunk simulation is as-expected
    {
        const auto reports = sim.get_all_simulation_reports();
        ASSERT_EQ(reports.size(), 2);
        ASSERT_EQ(reports.at(0).get_time(), SimulationClock::start());
        ASSERT_EQ(reports.at(1).get_time(), SimulationClock::start() + 1s);
    }
}
