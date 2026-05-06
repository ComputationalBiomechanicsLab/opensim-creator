#include "examples.h"

#include <libopynsim/opynsim.h>

#include <gtest/gtest.h>

using namespace opyn;

TEST(opynsim, examples_pendulum_specification_works)
{
    opyn::init();

    ASSERT_NO_THROW({ examples::pendulum_specification(); });
}

TEST(opynsim, examples_pendulum_model_works)
{
    opyn::init();

    ASSERT_NO_THROW({ examples::pendulum_model(); });
}

TEST(opynsim, examples_double_pendulum_specification_works)
{
    opyn::init();

    ASSERT_NO_THROW({ examples::double_pendulum_specification(); });
}

TEST(opynsim, examples_double_pendulum_model_works)
{
    opyn::init();

    ASSERT_NO_THROW({ examples::double_pendulum_model(); });
}
