#include "model_state.h"

#include <gtest/gtest.h>

using namespace opyn;

TEST(ModelState, can_default_construct)
{
    ASSERT_NO_THROW({ ModelState{}; });
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
