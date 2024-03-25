#include <oscar/Maths/Normalized.h>

#include <oscar/Maths/CommonFunctions.h>
#include <oscar/Maths/Constants.h>

#include <gtest/gtest.h>

using namespace osc;

TEST(Normalized, WorksForFloats)
{
    ASSERT_NO_THROW({ Normalized(0.5f); });
}

TEST(Normalized, ValueBelowZeroIsClampedToZero)
{
    static_assert(Normalized{-1.0f} == Normalized{0.0f});
}

TEST(Normalized, ValueAboveOneIsClampedToOne)
{
    static_assert(Normalized{1.5f} == Normalized{1.0f});
}

TEST(Normalized, NaNRemainsAsNaN)
{
    ASSERT_TRUE(std::isnan(Normalized{quiet_nan_v<float>}.get()));
}

TEST(Normalized, WorksAsIntendedForInterpolant)
{
    static_assert(lerp(0.0f, 1.0f, Normalized{0.0f}.get()) == 0.0f);
    static_assert(lerp(0.0f, 1.0f, Normalized{1.0f}.get()) == 1.0f);
}
