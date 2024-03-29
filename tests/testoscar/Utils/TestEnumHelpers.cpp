#include <oscar/Utils/EnumHelpers.h>

#include <gtest/gtest.h>

using namespace osc;

TEST(NumOptions, ReturnsValueOfNUMOPTIONSEnumMember)
{
    enum class SomeEnum { First, Second, Third, NUM_OPTIONS };
    static_assert(NumOptions<SomeEnum>() == 3);
}

TEST(NumOptions, ReturnsValueOfNUMOPTIONSWhenCustomized)
{
    enum class SomeCustomizedEnum { First, Second, Third, NUM_OPTIONS = 58 };
    static_assert(NumOptions<SomeCustomizedEnum>() == 58);
}

TEST(NumFlags, ReturnsValueOfNUMFLAGSEnumMember)
{
    enum class SomeFlagsEnum { First = 1<<0, Second = 1<<1, Third = 1<<2, NUM_FLAGS = 3};
    static_assert(NumFlags<SomeFlagsEnum>() == 3);
}
