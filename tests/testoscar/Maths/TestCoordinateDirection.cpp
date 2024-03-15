#include <oscar/Maths/CoordinateDirection.h>

#include <gtest/gtest.h>
#include <oscar/Shims/Cpp23/cstddef.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <concepts>

using namespace osc;
using namespace osc::literals;

TEST(CoordinateDirection, IsRegular)
{
    static_assert(std::regular<CoordinateDirection>);
}

TEST(CoordinateDirection, DefaultConstructedPointsInPositiveX)
{
    static_assert(CoordinateDirection{} == CoordinateDirection::x());
}

TEST(CoordinateDirection, XIsEquivalentToConstructingFromXAxisDirection)
{
    static_assert(CoordinateDirection::x() == CoordinateDirection{CoordinateAxis::x()});
}

TEST(CoordinateDirection, XYZAreNotEqualToEachover)
{
    static_assert(CoordinateDirection::x() != CoordinateDirection::y());
    static_assert(CoordinateDirection::x() != CoordinateDirection::z());
    static_assert(CoordinateDirection::y() != CoordinateDirection::z());
}

TEST(CoordinateDirection, PositiveDirectionsNotEqualToNegative)
{
    static_assert(CoordinateDirection::x() != CoordinateDirection::minus_x());
    static_assert(CoordinateDirection::y() != CoordinateDirection::minus_y());
    static_assert(CoordinateDirection::z() != CoordinateDirection::minus_z());
}

TEST(CoordinateDirection, AxisIgnoresPositiveVsNegative)
{
    static_assert(CoordinateDirection::x().axis() == CoordinateDirection::minus_x().axis());
    static_assert(CoordinateDirection::y().axis() == CoordinateDirection::minus_y().axis());
    static_assert(CoordinateDirection::z().axis() == CoordinateDirection::minus_z().axis());
}

TEST(CoordinateDirection, UnaryNegationWorksAsExpected)
{
    static_assert(-CoordinateDirection::x() == CoordinateDirection::minus_x());
    static_assert(-CoordinateDirection::y() == CoordinateDirection::minus_y());
    static_assert(-CoordinateDirection::z() == CoordinateDirection::minus_z());
    static_assert(-CoordinateDirection::minus_x() == CoordinateDirection::x());
    static_assert(-CoordinateDirection::minus_y() == CoordinateDirection::y());
    static_assert(-CoordinateDirection::minus_z() == CoordinateDirection::z());
}

TEST(CoordinateDirection, DirectionReturnsExpectedResults)
{
    static_assert(CoordinateDirection::x().direction() == 1.0f);
    static_assert(CoordinateDirection::minus_x().direction() == -1.0f);
    static_assert(CoordinateDirection::y().direction() == 1.0f);
    static_assert(CoordinateDirection::minus_y().direction() == -1.0f);
    static_assert(CoordinateDirection::z().direction() == 1.0f);
    static_assert(CoordinateDirection::minus_z().direction() == -1.0f);

    // templated (casted) versions
    static_assert(CoordinateDirection::x().direction<int>() == 1);
    static_assert(CoordinateDirection::x().direction<ptrdiff_t>() == 1_z);
    static_assert(CoordinateDirection::x().direction<double>() == 1.0);
    static_assert(CoordinateDirection::minus_x().direction<int>() == -1);
    static_assert(CoordinateDirection::minus_x().direction<ptrdiff_t>() == -1_z);
    static_assert(CoordinateDirection::minus_x().direction<double>() == -1.0);
}

TEST(CoordinateDirection, AreOrderedAsExpected)
{
    auto const expectedOrder = std::to_array({
        CoordinateDirection::minus_x(),
        CoordinateDirection::x(),
        CoordinateDirection::minus_y(),
        CoordinateDirection::y(),
        CoordinateDirection::minus_z(),
        CoordinateDirection::z(),
    });
    ASSERT_TRUE(std::is_sorted(expectedOrder.begin(), expectedOrder.end()));
}

TEST(CoordinateDirection, CrossWorksAsExpected)
{
    // cross products along same axis (undefined: falls back to first arg)
    static_assert(cross(CoordinateDirection::x(), CoordinateDirection::x()) == CoordinateDirection::x());
    static_assert(cross(CoordinateDirection::x(), CoordinateDirection::minus_x()) == CoordinateDirection::x());
    static_assert(cross(CoordinateDirection::minus_x(), CoordinateDirection::x()) == CoordinateDirection::minus_x());
    static_assert(cross(CoordinateDirection::y(), CoordinateDirection::y()) == CoordinateDirection::y());
    static_assert(cross(CoordinateDirection::y(), CoordinateDirection::minus_y()) == CoordinateDirection::y());
    static_assert(cross(CoordinateDirection::minus_y(), CoordinateDirection::y()) == CoordinateDirection::minus_y());
    static_assert(cross(CoordinateDirection::z(), CoordinateDirection::z()) == CoordinateDirection::z());
    static_assert(cross(CoordinateDirection::z(), CoordinateDirection::minus_z()) == CoordinateDirection::z());
    static_assert(cross(CoordinateDirection::minus_z(), CoordinateDirection::z()) == CoordinateDirection::minus_z());

    // +X on lhs
    static_assert(cross(CoordinateDirection::x(), CoordinateDirection::y()) == CoordinateDirection::z());
    static_assert(cross(CoordinateDirection::x(), CoordinateDirection::minus_y()) == CoordinateDirection::minus_z());
    static_assert(cross(CoordinateDirection::x(), CoordinateDirection::z()) == CoordinateDirection::minus_y());
    static_assert(cross(CoordinateDirection::x(), CoordinateDirection::minus_z()) == CoordinateDirection::y());

    // -X on lhs
    static_assert(cross(CoordinateDirection::minus_x(), CoordinateDirection::y()) == CoordinateDirection::minus_z());
    static_assert(cross(CoordinateDirection::minus_x(), CoordinateDirection::minus_y()) == CoordinateDirection::z());
    static_assert(cross(CoordinateDirection::minus_x(), CoordinateDirection::z()) == CoordinateDirection::y());
    static_assert(cross(CoordinateDirection::minus_x(), CoordinateDirection::minus_z()) == CoordinateDirection::minus_y());

    // +Y on lhs
    static_assert(cross(CoordinateDirection::y(), CoordinateDirection::z()) == CoordinateDirection::x());
    static_assert(cross(CoordinateDirection::y(), CoordinateDirection::minus_z()) == CoordinateDirection::minus_x());
    static_assert(cross(CoordinateDirection::y(), CoordinateDirection::x()) == CoordinateDirection::minus_z());
    static_assert(cross(CoordinateDirection::y(), CoordinateDirection::minus_x()) == CoordinateDirection::z());

    // -Y on lhs
    static_assert(cross(CoordinateDirection::minus_y(), CoordinateDirection::z()) == CoordinateDirection::minus_x());
    static_assert(cross(CoordinateDirection::minus_y(), CoordinateDirection::minus_z()) == CoordinateDirection::x());
    static_assert(cross(CoordinateDirection::minus_y(), CoordinateDirection::x()) == CoordinateDirection::z());
    static_assert(cross(CoordinateDirection::minus_y(), CoordinateDirection::minus_x()) == CoordinateDirection::minus_z());

    // +Z on lhs
    static_assert(cross(CoordinateDirection::z(), CoordinateDirection::x()) == CoordinateDirection::y());
    static_assert(cross(CoordinateDirection::z(), CoordinateDirection::minus_x()) == CoordinateDirection::minus_y());
    static_assert(cross(CoordinateDirection::z(), CoordinateDirection::y()) == CoordinateDirection::minus_x());
    static_assert(cross(CoordinateDirection::z(), CoordinateDirection::minus_y()) == CoordinateDirection::x());

    // -Z on lhs
    static_assert(cross(CoordinateDirection::minus_z(), CoordinateDirection::x()) == CoordinateDirection::minus_y());
    static_assert(cross(CoordinateDirection::minus_z(), CoordinateDirection::minus_x()) == CoordinateDirection::y());
    static_assert(cross(CoordinateDirection::minus_z(), CoordinateDirection::y()) == CoordinateDirection::x());
    static_assert(cross(CoordinateDirection::minus_z(), CoordinateDirection::minus_y()) == CoordinateDirection::minus_x());
}
