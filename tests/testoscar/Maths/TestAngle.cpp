#include "oscar/Maths/Angle.h"

#include <gtest/gtest.h>
#include <oscar/Maths/MathHelpers.h>

#include <numbers>

using namespace osc;
using namespace osc::literals;

TEST(Angle, RadiansCanValueConstructToZero)
{
    static_assert(Radians{} == Radians{0.0f});
}

TEST(Angle, RadiansCanConstructWithInitialValue)
{
    static_assert(Radians{5.0f}.count() == 5.0);
}

TEST(Angle, RadiansCanConstructViaUDL)
{
    static_assert(8.0_rad == Radians{8.0f});
}

TEST(Angle, RadiansCanConstructFromDegrees)
{
    static_assert(Radians{50.0_deg} == Radians{Degrees{50.0f}});
}

TEST(Angle, RadiansConversionFromDegreesWorksAsExpected)
{
    constexpr auto pi = std::numbers::pi_v<float>;

    static_assert(Radians{-90.0_deg} == Radians{-0.5f*pi});
    static_assert(Radians{90.0_deg} == Radians{0.5f*pi});

    static_assert(Radians{-180.0_deg} == Radians{-pi});
    static_assert(Radians{180.0_deg} == Radians{pi});

    static_assert(Radians{-360.0_deg} == Radians{-2.0f*pi});
    static_assert(Radians{360.0_deg} == Radians{2.0f*pi});
}

TEST(Angle, RadiansAddingTwoRadiansWorksAsExpected)
{
    static_assert(Radians{1.0f} + Radians{1.0f} == Radians{2.0f});
}

TEST(Angle, RadiansSubtractingTwoRadiansWorksAsExpected)
{
    static_assert(Radians{1.0f} - Radians{0.5f} == Radians{0.5f});
}

TEST(Angle, RadiansMultiplyingRadiansByScalarWorksAsExpected)
{
    static_assert(2.0f*Radians{1.0f} == Radians{2.0f});
    static_assert(Radians{1.0f}*3.0f == Radians{3.0f});
}

TEST(Angle, ComparisonBehavesAsExpected)
{
    static_assert(1_rad < 2_rad);
    static_assert(1_rad <= 2_rad);
    static_assert(1_rad == 1_rad);
    static_assert(1_rad >= 1_rad);
    static_assert(1_rad > 0.5_rad);
}

TEST(Angle, RadiansPlusEqualsWorksAsExpected)
{
    Radians r{1.0f};
    r += 1_rad;
    ASSERT_EQ(r, 2_rad);
}

TEST(Angle, FmodBehavesAsExpected)
{
    using osc::fmod;

    auto r = 2_rad;
    r = fmod(r, 1_rad);
    ASSERT_EQ(r, 0_rad);
}

TEST(Angle, MixedAdditionReturnsRadians)
{
    static_assert(Radians{360_deg} == Radians{180_deg} + 180_deg);
}

TEST(Angle, TurnConvertsToRadiansOrDegreesAsExpected)
{
    constexpr auto pi = std::numbers::pi_v<float>;

    static_assert(1_turn == 2.0*pi*1_rad);
    static_assert(0.5_turn == pi*1_rad);
    static_assert(1_turn == 360_deg);
    static_assert(0.5_turn == 180_deg);
}

TEST(Angle, CanCompareMixedAngularTypes)
{
    static_assert(1_turn == 360_deg);  // should compile
}

TEST(Angle, ScalarDivisionWorksAsExpected)
{
    static_assert(1_turn/2 == 180_deg);
    static_assert(360*360/360_deg == 1/1_turn);
}
