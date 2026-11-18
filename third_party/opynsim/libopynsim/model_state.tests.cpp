#include "model_state.h"

#include <libopynsim/examples.h>
#include <libopynsim/model.h>
#include <libopynsim/symbol.h>

#include <gtest/gtest.h>
#include <SimTKcommon/internal/State.h>

#include <format>
#include <string>

using namespace opyn;

TEST(ModelState, can_default_construct)
{
    ASSERT_NO_THROW({ ModelState{}; });
}

TEST(ModelState, equality_returns_true_in_trivial_case)
{
    ASSERT_EQ(ModelState{}, ModelState{});
}

TEST(ModelState, mutating_state_q_changes_equivalence)
{
    const Model model = examples::double_pendulum_model();
    const ModelState initial_state = model.initial_state(ModelStateStage::report);
    ModelState state = initial_state;

    ASSERT_EQ(state, initial_state);
    ASSERT_TRUE(state.upd_simbody_state().getQ().size() > 0);
    state.upd_simbody_state().updQ() += 0.5;  // modify Q
    ASSERT_NE(state, initial_state);
}

TEST(ModelState, has_blank_attrs_when_default_constructed)
{
    const ModelState model_state;
    ASSERT_TRUE(model_state.attrs().empty());
}

TEST(ModelState, set_attrs_sets_attrs)
{
    ModelState model_state;
    const ModelState::attrs_type new_attrs = {{Symbol{"a"}, 1.0}, {Symbol{"b"}, -5.0}};

    ASSERT_NE(model_state.attrs(), new_attrs);
    model_state.set_attrs(new_attrs);
    ASSERT_EQ(model_state.attrs(), new_attrs);
}

TEST(ModelState, default_constructed_can_be_formatted_to_a_string)
{
    const std::string str = std::format("{}", ModelState{});
    ASSERT_EQ(str, "ModelState(time=0.0, stage=ModelStateStage.TIME)");
}
