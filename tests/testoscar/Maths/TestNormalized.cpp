#include <oscar/Maths/Normalized.h>

#include <oscar/Maths/CommonFunctions.h>
#include <oscar/Maths/Constants.h>

#include <gtest/gtest.h>

using namespace osc;

TEST(Normalized, works_for_floats)
{
    ASSERT_NO_THROW({ Normalized(0.5f); });
}

TEST(Normalized, value_below_zero_is_clamped_to_zero)
{
    static_assert(Normalized{-1.0f} == Normalized{0.0f});
}

TEST(Normalized, value_above_one_is_clamped_to_one)
{
    static_assert(Normalized{1.5f} == Normalized{1.0f});
}

TEST(Normalized, NaN_remains_NaN)
{
    ASSERT_TRUE(std::isnan(Normalized{quiet_nan_v<float>}.get()));
}

TEST(Normalized, works_as_intended_as_an_interpolant_for_lerp)
{
    static_assert(lerp(0.0f, 1.0f, Normalized{0.0f}.get()) == 0.0f);
    static_assert(lerp(0.0f, 1.0f, Normalized{1.0f}.get()) == 1.0f);
}
