#include "oscar/Maths/Angle.h"

#include <gtest/gtest.h>
#include <oscar/Maths/MathHelpers.h>

#include <numbers>

using namespace osc;
using namespace osc::literals;

TEST(Radians, can_be_constructed_from_zero_float)
{
    static_assert(Radians{} == Radians{0.0f});
}

TEST(Radians, can_be_constructed_with_initial_value)
{
    static_assert(Radians{5.0f}.count() == 5.0);
}

TEST(Radians, can_be_constructed_from_user_defined_radians_literal)
{
    static_assert(8.0_rad == Radians{8.0f});
}

TEST(Radians, can_be_constructed_from_Degrees)
{
    static_assert(Radians{50.0_deg} == Radians{Degrees{50.0f}});
}

TEST(Radians, construction_from_Degrees_converts_the_value_as_expected)
{
    static_assert(Radians{-90.0_deg} == Radians{-0.5f*pi_v<float>});
    static_assert(Radians{90.0_deg} == Radians{0.5f*pi_v<float>});

    static_assert(Radians{-180.0_deg} == Radians{-pi_v<float>});
    static_assert(Radians{180.0_deg} == Radians{pi_v<float>});

    static_assert(Radians{-360.0_deg} == Radians{-2.0f*pi_v<float>});
    static_assert(Radians{360.0_deg} == Radians{2.0f*pi_v<float>});
}

TEST(Radians, addition_operator_works_as_expected)
{
    static_assert(Radians{1.0f} + Radians{1.0f} == Radians{2.0f});
}

TEST(Radians, subtraction_operator_works_as_expected)
{
    static_assert(Radians{1.0f} - Radians{0.5f} == Radians{0.5f});
}

TEST(Radians, scalar_multiplication_operator_works_as_expected)
{
    static_assert(2.0f*Radians{1.0f} == Radians{2.0f});
    static_assert(Radians{1.0f}*3.0f == Radians{3.0f});
}

TEST(Radians, three_way_comparison_works_as_expected)
{
    static_assert(1_rad < 2_rad);
    static_assert(1_rad <= 2_rad);
    static_assert(1_rad == 1_rad);
    static_assert(1_rad >= 1_rad);
    static_assert(1_rad > 0.5_rad);
}

TEST(Radians, addition_assignment_operator_works_as_expected)
{
    Radians r{1.0f};
    r += 1_rad;
    ASSERT_EQ(r, 2_rad);
}

TEST(Radians, mod_works_as_expected)
{
    using osc::mod;

    auto r = 2_rad;
    r = mod(r, 1_rad);
    ASSERT_EQ(r, 0_rad);
}

TEST(Angle, addition_operator_works_with_different_Angle_types)
{
    static_assert(Radians{360_deg} == Radians{180_deg} + 180_deg);
}

TEST(Turns, converts_to_Degrees_or_Radians_as_expected)
{
    static_assert(1_turn == 2.0f*pi_v<float>*1_rad);
    static_assert(0.5_turn == pi_v<float>*1_rad);
    static_assert(1_turn == 360_deg);
    static_assert(0.5_turn == 180_deg);
}

TEST(Angle, equality_works_across_mixed_Angle_types)
{
    static_assert(1_turn == 360_deg);  // should compile
}

TEST(Turn, division_by_a_scalar_works_as_expected)
{
    static_assert(1_turn/2 == 180_deg);
}

TEST(Angle, is_compatible_with_projected_clamp_algorithm)
{
    // just a "it'd be nice to know osc::Angle inter-operates with std::ranges::clamp algs"
    struct S {
        Degrees ang;
    };
    static_assert(clamp(S{-10_deg}, S{0_deg}, S{180_deg}, {}, &S::ang).ang == S{0_deg}.ang);
}

TEST(Angle, std_is_convertible_to_works_between_angles)
{
    static_assert(std::convertible_to<Radians, Degrees>);
    static_assert(std::convertible_to<Degrees, Radians>);
    static_assert(std::convertible_to<Degrees, Turns>);
    // etc. - many parts of the codebase require that these are automatically convertible
}
