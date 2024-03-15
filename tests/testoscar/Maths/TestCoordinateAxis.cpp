#include <oscar/Maths/CoordinateAxis.h>

#include <gtest/gtest.h>
#include <oscar/Maths/GeometricFunctions.h>
#include <oscar/Maths/Vec3.h>

#include <concepts>

using namespace osc;

TEST(CoordinateAxis, IsRegular)
{
    static_assert(std::regular<CoordinateAxis>);
}

TEST(CoordinateAxis, WhenDefaultConstructedIsEqualToXAxis)
{
    static_assert(CoordinateAxis{} == CoordinateAxis::x());
}

TEST(CoordinateAxis, XYZAreEqualToThemselves)
{
    static_assert(CoordinateAxis::x() == CoordinateAxis::x());
    static_assert(CoordinateAxis::y() == CoordinateAxis::y());
    static_assert(CoordinateAxis::z() == CoordinateAxis::z());
}

TEST(CoordinateAxis, AxesArentEqualToEachover)
{
    static_assert(CoordinateAxis::x() != CoordinateAxis::y());
    static_assert(CoordinateAxis::x() != CoordinateAxis::z());
    static_assert(CoordinateAxis::y() != CoordinateAxis::z());
}

TEST(CoordinateAxis, HasExpectedAxisWhenConstructedFromInteger)
{
    static_assert(CoordinateAxis{0} == CoordinateAxis::x());
    static_assert(CoordinateAxis{1} == CoordinateAxis::y());
    static_assert(CoordinateAxis{2} == CoordinateAxis::z());
}

TEST(CoordinateAxis, AreTotallyOrdered)
{
    static_assert(CoordinateAxis::x() < CoordinateAxis::y());
    static_assert(CoordinateAxis::y() < CoordinateAxis::z());
    static_assert(CoordinateAxis::x() < CoordinateAxis::z());  // transitive
}

TEST(CoordinateAxis, XIndexReturnsExpectedResults)
{
    static_assert(CoordinateAxis::x().index() == 0);
    static_assert(CoordinateAxis::y().index() == 1);
    static_assert(CoordinateAxis::z().index() == 2);
}

TEST(CoordinateAxis, NextWorksAsExpected)
{
    static_assert(CoordinateAxis::x().next() == CoordinateAxis::y());
    static_assert(CoordinateAxis::y().next() == CoordinateAxis::z());
    static_assert(CoordinateAxis::z().next() == CoordinateAxis::x());
}

TEST(CoordinateAxis, PrevWorksAsExpected)
{
    static_assert(CoordinateAxis::x().previous() == CoordinateAxis::z());
    static_assert(CoordinateAxis::y().previous() == CoordinateAxis::x());
    static_assert(CoordinateAxis::z().previous() == CoordinateAxis::y());
}
