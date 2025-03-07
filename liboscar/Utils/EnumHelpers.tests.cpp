#include "EnumHelpers.h"

#include <gtest/gtest.h>

#include <array>

using namespace osc;
TEST(num_options, returns_value_of_NUM_OPTIONS_enum_member)
{
    enum class SomeEnum { First, Second, Third, NUM_OPTIONS };
    static_assert(num_options<SomeEnum>() == 3);
}

TEST(num_options, returns_value_of_NUM_OPTIONS_even_if_its_customized)
{
    enum class SomeCustomizedEnum { First, Second, Third, NUM_OPTIONS = 58 };
    static_assert(num_options<SomeCustomizedEnum>() == 58);
}

TEST(num_flags, returns_value_of_NUM_FLAGS_enum_member)
{
    enum class SomeFlagsEnum : unsigned { First = 1<<0, Second = 1<<1, Third = 1<<2, NUM_FLAGS = 3};
    static_assert(num_flags<SomeFlagsEnum>() == 3);
}
