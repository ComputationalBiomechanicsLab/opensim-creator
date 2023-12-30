#include <oscar/Graphics/Unorm8.hpp>

#include <gtest/gtest.h>
#include <oscar/Maths/Vec.hpp>
#include <oscar/Maths/Vec3.hpp>

#include <limits>

using osc::Unorm8;
using osc::Vec;
using osc::Vec3;

TEST(Unorm8, ComparisonBetweenBytesWorksAsExpected)
{
    static_assert(Unorm8{static_cast<std::byte>(0xfa)} == Unorm8{static_cast<std::byte>(0xfa)});
}

TEST(Unorm8, ComparisonBetweenConvertedFloatsWorksAsExpected)
{
    static_assert(Unorm8{0.5f} == Unorm8{0.5f});
}

TEST(Unorm8, NaNsAreConvertedToZero)
{
    static_assert(Unorm8(std::numeric_limits<float>::quiet_NaN()) == Unorm8{0.0f});
}

TEST(Unorm8, CanCreateVec3OfUnormsFromUsualVec3OfFloats)
{
    Vec3 const v{0.25f, 1.0f, 1.5f};
    Vec<3, Unorm8> const converted{v};
    Vec<3, Unorm8> const expected{Unorm8{0.25f}, Unorm8{1.0f}, Unorm8{1.5f}};
    ASSERT_EQ(converted, expected);
}

TEST(Unorm8, CanCreateUsualVec3FromVec3OfUNorms)
{
    Vec<3, Unorm8> const v{Unorm8{0.1f}, Unorm8{0.2f}, Unorm8{0.3f}};
    Vec3 const converted{v};
    Vec3 const expected{Unorm8{0.1f}.normalized(), Unorm8{0.2f}.normalized(), Unorm8{0.3f}.normalized()};
    ASSERT_EQ(converted, expected);
}
