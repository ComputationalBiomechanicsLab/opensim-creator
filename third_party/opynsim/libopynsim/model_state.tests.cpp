#include "model_state.h"

#include <gtest/gtest.h>

using namespace opyn;

TEST(ModelState, can_default_construct)
{
    ASSERT_NO_THROW({ ModelState{}; });
}
