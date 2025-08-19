#include "Unorm8.h"

#include <liboscar/Maths/Vec.h>
#include <liboscar/Maths/Vector3.h>
#include <liboscar/Utils/Algorithms.h>
#include <liboscar/Utils/Conversion.h>

#include <gtest/gtest.h>

#include <functional>
#include <limits>
#include <sstream>
#include <type_traits>

using namespace osc;

TEST(Unorm, is_not_trivially_constructible)
{
    static_assert(not std::is_trivially_constructible_v<Unorm8>);
}

TEST(Unorm8, default_constructs_to_zero)
{
    static_assert(Unorm8{} == Unorm8{0});
}
TEST(Unorm8, compares_equivalent_to_a_std_byte_with_the_same_value)
{
    static_assert(to<Unorm8>(static_cast<std::byte>(0xfa)) == to<Unorm8>(static_cast<std::byte>(0xfa)));
}

TEST(Unorm8, compares_equivalent_to_another_Unorm8_with_the_same_floating_point_value)
{
    static_assert(Unorm8{0.5f} == Unorm8{0.5f});
}

TEST(Unorm8, floating_point_NaNs_convert_to_zero)
{
    // because the underlying integer-based storage can't encode NaNs
    static_assert(Unorm8{std::numeric_limits<float>::quiet_NaN()} == Unorm8{0.0f});
}

TEST(Unorm8, can_construct_a_Vector3_of_Unorm8s_from_a_Vector3_of_floats)
{
    // this is useful for (e.g.) color conversion and quantizing mesh data

    const Vector3 vector3_of_floats{0.25f, 1.0f, 1.5f};
    const Vec<3, Unorm8> vector3_of_unorm8s{vector3_of_floats};
    const Vec<3, Unorm8> expected_content{Unorm8{0.25f}, Unorm8{1.0f}, Unorm8{1.5f}};
    ASSERT_EQ(vector3_of_unorm8s, expected_content);
}

TEST(Unorm8, can_construct_a_Vector3_of_floats_from_a_Vector3_of_Unorm8s)
{
    // this is useful for (e.g.) color conversion and quantizing mesh data

    const Vec<3, Unorm8> vector3_of_unorm8s{Unorm8{0.1f}, Unorm8{0.2f}, Unorm8{0.3f}};
    const Vector3 vector3_of_floats{vector3_of_unorm8s};
    const Vector3 expected_content{Unorm8{0.1f}.normalized_value(), Unorm8{0.2f}.normalized_value(), Unorm8{0.3f}.normalized_value()};
    ASSERT_EQ(vector3_of_floats, expected_content);
}

TEST(Unorm8, converts_midpoint_from_a_std_byte_as_expected)
{
    ASSERT_EQ(Unorm8{0.5f}, to<Unorm8>(static_cast<std::byte>(127)));
}

TEST(Unorm8, value_type_typedef_returns_uint8_t)
{
    static_assert(std::same_as<Unorm8::value_type, uint8_t>);
}

TEST(Unorm8, can_be_written_to_a_std_ostream)
{
    std::stringstream ss;
    ss << Unorm8{};
    ASSERT_FALSE(ss.str().empty());
}

TEST(Unorm8, can_be_hashed_with_std_hash)
{
    const Unorm8 value{0x48};
    ASSERT_NE(std::hash<Unorm8>{}(value), 0);
}

TEST(Unorm8, lerp_works_as_expected)
{
    ASSERT_EQ(lerp(Unorm8{0x00}, Unorm8{0xff}, 0.0f), Unorm8{0x00});
    ASSERT_EQ(lerp(Unorm8{0x00}, Unorm8{0xff}, 1.0f), Unorm8{0xff});
    ASSERT_EQ(lerp(Unorm8{0x00}, Unorm8{0xff}, 0.5f), Unorm8{127});
}

TEST(Unorm8, clamp_works_as_expected)
{
    static_assert(clamp(Unorm8{10}, Unorm8{0}, Unorm8{255}) == Unorm8{10});
    static_assert(clamp(Unorm8{10}, Unorm8{15}, Unorm8{255}) == Unorm8{15});
    static_assert(clamp(Unorm8{10}, Unorm8{0}, Unorm8{8}) == Unorm8{8});
}

TEST(Unorm8, saturate_returns_provided_Unorm)
{
    // a `Unorm<T>` is saturated by design.

    static_assert(saturate(Unorm8{0x00}) == Unorm8{0x00});
    static_assert(saturate(Unorm8{0xfe}) == Unorm8{0xfe});
    static_assert(saturate(Unorm8{0xff}) == Unorm8{0xff});
}
