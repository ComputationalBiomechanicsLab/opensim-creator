#include <oscar/Utils/EnumHelpers.hpp>

#include <gtest/gtest.h>

using osc::NumFlags;
using osc::NumOptions;

TEST(NumOptions, ReturnsValueOfNUM_OPTIONSEnumMember)
{
    enum class SomeEnum { First, Second, Third, NUM_OPTIONS };
    static_assert(NumOptions<SomeEnum>() == 3);
}

TEST(NumOptions, ReturnsValueOfNUM_OPTIONSWhenCustomized)
{
    enum class SomeCustomizedEnum { First, Second, Third, NUM_OPTIONS = 58 };
    static_assert(NumOptions<SomeCustomizedEnum>() == 58);
}

TEST(NumFlags, ReturnsValueOfNUM_FLAGSEnumMember)
{
    enum class SomeFlagsEnum { First = 1<<0, Second = 1<<1, Third = 1<<2, NUM_FLAGS = 3};
    static_assert(NumFlags<SomeFlagsEnum>() == 3);
}
