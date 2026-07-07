#include "forward_dynamics_solver.h"

#include <libopynsim/examples.h>
#include <libopynsim/model_state.h>
#include <libopynsim/model_state_stage.h>
#include <libopynsim/opynsim.h>

#include <gtest/gtest.h>

#include <vector>

using namespace opyn;

TEST(ForwardDynamicsSimulation, works_for_basic_pendulum_example)
{
    opyn::init();

    // Simulate a pendulum between `[0, 1.0]` seconds, sampling each `0.1` seconds.
    const auto pendulum = examples::pendulum_model();
    const auto state = pendulum.initial_state();
    auto simulation = ForwardDynamicsSolver{pendulum, state};
    std::vector<ModelState> states;
    for (size_t i = 0; i <= 10; ++i) {
        states.push_back(simulation.integrate_to(static_cast<double>(i)*0.1, ModelStateStage::dynamics));
    }

    // Ensure each state has the correct time and realization level.
    for (size_t i = 0; i <= 10; ++i) {
        ASSERT_EQ(states.at(i).time(), static_cast<double>(i)*0.1);
        ASSERT_GE(states.at(i).stage(), ModelStateStage::dynamics);
    }
}
