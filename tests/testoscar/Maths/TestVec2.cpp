#include <oscar/Maths/Vec2.h>

#include <gtest/gtest.h>

using namespace osc;

TEST(Vec2, WithElementWorksAsExpected)
{
    ASSERT_EQ(Vec2{}.with_element(0, 2.0f), Vec2(2.0f, 0.0f));
    ASSERT_EQ(Vec2(1.0f).with_element(0, 3.0f), Vec2(3.0f, 1.0f));
    ASSERT_EQ(Vec2{}.with_element(1, 3.0f), Vec2(0.0f, 3.0f));
}
