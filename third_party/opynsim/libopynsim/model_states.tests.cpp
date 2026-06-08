#include "model_states.h"

#include <libopynsim/model_state.h>

#include <gtest/gtest.h>

#include <memory>

using namespace opyn;

TEST(ModelStates, default_constructor_returns_empty_sequence)
{
    const ModelStates model_states;
    ASSERT_EQ(model_states.size(), 0);
}

TEST(ModelStates, handle_push_back_pushes_handle_to_end_of_sequence)
{
    ModelStates model_states;

    const auto handle1 = std::make_shared<ModelState>();
    model_states.handle_push_back(handle1);
    ASSERT_EQ(model_states.size(), 1);
    ASSERT_EQ(handle1.use_count(), 2);

    const auto handle2 = std::make_shared<ModelState>();
    model_states.handle_push_back(handle2);
    ASSERT_EQ(model_states.size(), 2);
    ASSERT_EQ(handle1.use_count(), 2);
    ASSERT_EQ(handle2.use_count(), 2);

    ASSERT_EQ(model_states.handle_at(0), handle1);
    ASSERT_EQ(model_states.handle_at(1), handle2);
}

TEST(ModelStates, handle_at_throws_if_given_out_of_bounds_index)
{
    ModelStates model_states;

    model_states.handle_push_back(std::make_shared<ModelState>());

    ASSERT_NO_THROW({ model_states.handle_at(0); });
    ASSERT_THROW({ model_states.handle_at(1); }, std::out_of_range);
}

TEST(ModelStates, begin_dereferences_first_handle)
{
    ModelStates model_states;
    const auto handle1 = std::make_shared<ModelState>();
    model_states.handle_push_back(handle1);
    const auto it = model_states.begin();
    const ModelState& state = *it;

    ASSERT_EQ(&state, handle1.get());
}

TEST(ModelStates, to_handle_list_returns_vector_of_handles_in_order)
{
    ModelStates model_states;
    const auto handle1 = std::make_shared<ModelState>();
    const auto handle2 = std::make_shared<ModelState>();
    const auto handle3 = std::make_shared<ModelState>();

    model_states.handle_push_back(handle1);
    model_states.handle_push_back(handle2);
    model_states.handle_push_back(handle3);

    const auto handle_list = model_states.to_handle_list();
    ASSERT_EQ(handle_list.size(), model_states.size());
    for (size_t i = 0; i < handle_list.size(); ++i) {
        ASSERT_EQ(handle_list[i], model_states.handle_at(i));
    }
}
