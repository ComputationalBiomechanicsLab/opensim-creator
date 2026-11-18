#include "forward_dynamics_solver.h"

#include <libopynsim/examples.h>
#include <libopynsim/model.h>
#include <libopynsim/model_state.h>
#include <libopynsim/model_state_stage.h>
#include <libopynsim/opynsim.h>

#include <gtest/gtest.h>

#include <cstddef>
#include <vector>

using namespace opyn;

TEST(ForwardDynamicsSolver, works_for_basic_pendulum_example)
{
    opyn::init();

    constexpr size_t num_samples = 5;
    constexpr double step_size = 0.1;

    // Simulate a pendulum, sampling each step
    const Model pendulum = examples::pendulum_model();
    const ModelState state = pendulum.initial_state();
    ForwardDynamicsSolver solver{pendulum, state};
    std::vector<ModelState> states;
    for (size_t i = 0; i < num_samples; ++i) {
        states.push_back(
            solver.integrate_to(static_cast<double>(i) * step_size, ModelStateStage::dynamics));
    }

    // Ensure each state has the correct time and realization level.
    for (size_t i = 0; i < num_samples; ++i) {
        ASSERT_EQ(states.at(i).time(), static_cast<double>(i) * step_size);
        ASSERT_GE(states.at(i).stage(), ModelStateStage::dynamics);
    }
}

TEST(ForwardDynamicsSolver, constructing_without_initial_state_uses_model_initial_state)
{
    opyn::init();

    const Model pendulum = examples::pendulum_model();
    const ForwardDynamicsSolver solver{pendulum};

    ASSERT_EQ(solver.model_state(), pendulum.initial_state());
}

TEST(ForwardDynamicsSolver, calling_integrate_to_mutates_model_state)
{
    opyn::init();

    const Model pendulum = examples::pendulum_model();
    ForwardDynamicsSolver solver{pendulum};
    solver.integrate_to(0.1);

    ASSERT_NE(solver.model_state(), pendulum.initial_state());
}

TEST(ForwardDynamicsSolver, the_copy_returned_by_integrate_to_is_equivalent_to_the_internal_state)
{
    opyn::init();

    const Model pendulum = examples::pendulum_model();
    ForwardDynamicsSolver solver{pendulum};
    const ModelState rv = solver.integrate_to(0.1);

    ASSERT_EQ(rv, solver.model_state());
}

TEST(ForwardDynamicsSolver, supplying_a_state_to_the_ctor_uses_that_state)
{
    opyn::init();

    const Model pendulum = examples::pendulum_model();
    const ModelState advanced_state = ForwardDynamicsSolver{pendulum}.integrate_to(0.1);
    ForwardDynamicsSolver solver{pendulum, advanced_state};

    ASSERT_EQ(solver.model_state(), advanced_state);
    ASSERT_EQ(solver.integrate_to(0.2), ForwardDynamicsSolver{pendulum}.integrate_to(0.2));
}
