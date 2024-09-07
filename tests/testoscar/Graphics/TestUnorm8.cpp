#include <oscar/Graphics/Unorm8.h>

#include <gtest/gtest.h>
#include <oscar/Maths/Vec.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/Utils/Algorithms.h>
#include <oscar/Utils/Conversion.h>

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
TEST(Unorm8, ComparisonBetweenBytesWorksAsExpected)
{
    static_assert(to<Unorm8>(static_cast<std::byte>(0xfa)) == to<Unorm8>(static_cast<std::byte>(0xfa)));
}

TEST(Unorm8, ComparisonBetweenConvertedFloatsWorksAsExpected)
{
    static_assert(Unorm8{0.5f} == Unorm8{0.5f});
}

TEST(Unorm8, NaNsAreConvertedToZero)
{
    static_assert(Unorm8{std::numeric_limits<float>::quiet_NaN()} == Unorm8{0.0f});
}

TEST(Unorm8, CanCreateVec3OfUnormsFromUsualVec3OfFloats)
{
    const Vec3 v{0.25f, 1.0f, 1.5f};
    const Vec<3, Unorm8> converted{v};
    const Vec<3, Unorm8> expected{Unorm8{0.25f}, Unorm8{1.0f}, Unorm8{1.5f}};
    ASSERT_EQ(converted, expected);
}

TEST(Unorm8, CanCreateUsualVec3FromVec3OfUNorms)
{
    const Vec<3, Unorm8> v{Unorm8{0.1f}, Unorm8{0.2f}, Unorm8{0.3f}};
    const Vec3 converted{v};
    const Vec3 expected{Unorm8{0.1f}.normalized_value(), Unorm8{0.2f}.normalized_value(), Unorm8{0.3f}.normalized_value()};
    ASSERT_EQ(converted, expected);
}

TEST(Unorm8, ConvertsAsExpected)
{
    ASSERT_EQ(Unorm8{0.5f}, to<Unorm8>(static_cast<std::byte>(127)));
}

TEST(Unorm8, value_type_returns_uint8_t)
{
    static_assert(std::same_as<Unorm8::value_type, uint8_t>);
}

TEST(Unorm8, can_be_streamed_to_ostream)
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
