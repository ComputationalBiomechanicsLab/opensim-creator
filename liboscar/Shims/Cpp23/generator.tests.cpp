#include "generator.h"

#include <gtest/gtest.h>

using namespace osc;

namespace
{
    cpp23::generator<int> coroutine_that_returns_1()
    {
        co_yield 1;
    }

    cpp23::generator<int> coroutine_that_returns_1_2_3()
    {
        co_yield 1;
        co_yield 2;
        co_yield 3;
    }
}

TEST(generator, can_yield_a_single_value)
{
    auto generator = coroutine_that_returns_1();
    auto it = generator.begin();
    ASSERT_NE(it, generator.end());
    ASSERT_EQ(*it, 1);
    ++it;
    ASSERT_EQ(it, generator.end());
}

TEST(generator, can_yield_three_values)
{
    auto generator = coroutine_that_returns_1_2_3();
    auto it = generator.begin();

    ASSERT_NE(it, generator.end());
    ASSERT_EQ(*it, 1);
    ++it;
    ASSERT_EQ(*it, 2);
    ++it;
    ASSERT_EQ(*it, 3);
    ++it;
    ASSERT_EQ(it, generator.end());
}
