#include "CollisionTests.h"

#include <liboscar/Maths/Rect.h>
#include <liboscar/Maths/Vec2.h>

#include <gtest/gtest.h>

using namespace osc;

TEST(is_intersecting, rect_point_works_as_expected)
{
    const Rect rect{Vec2{-1.0f}, Vec2{+1.0f}};

    // test on-the -origin/-edge cases
    ASSERT_TRUE(is_intersecting(rect, Vec2( 0.0f,  0.0f)));
    ASSERT_TRUE(is_intersecting(rect, Vec2(-1.0f, -1.0f)));
    ASSERT_TRUE(is_intersecting(rect, Vec2(-1.0f,  1.0f)));
    ASSERT_TRUE(is_intersecting(rect, Vec2( 1.0f,  1.0f)));
    ASSERT_TRUE(is_intersecting(rect, Vec2( 1.0f, -1.0f)));

    // twiddle around the origin (all are within the `Rect`)
    ASSERT_TRUE(is_intersecting(rect, Vec2(-0.1f,  0.0f)));
    ASSERT_TRUE(is_intersecting(rect, Vec2( 0.1f,  0.0f)));
    ASSERT_TRUE(is_intersecting(rect, Vec2( 0.0f, -0.1f)));
    ASSERT_TRUE(is_intersecting(rect, Vec2( 0.0f,  0.1f)));

    // twiddle around the bottom-left
    ASSERT_FALSE(is_intersecting(rect, Vec2(-1.1f, -1.0f)));
    ASSERT_FALSE(is_intersecting(rect, Vec2(-1.0f, -1.1f)));
    ASSERT_TRUE(is_intersecting(rect, Vec2(-1.0f, -0.9f)));
    ASSERT_TRUE(is_intersecting(rect, Vec2(-0.9f, -1.0f)));
}
