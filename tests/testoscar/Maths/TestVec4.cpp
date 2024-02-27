#include <oscar/Maths/Vec4.h>

#include <gtest/gtest.h>

using namespace osc;

TEST(Vec4, WithElementWorksAsExpected)
{
    ASSERT_EQ(Vec4{}.with_element(0, 2.0f), Vec4(2.0f, 0.0f, 0.0f, 0.0f));
    ASSERT_EQ(Vec4(1.0f).with_element(0, 3.0f), Vec4(3.0f, 1.0f, 1.0f, 1.0f));
    ASSERT_EQ(Vec4{}.with_element(1, 3.0f), Vec4(0.0f, 3.0f, 0.0f, 0.0f));
    ASSERT_EQ(Vec4{}.with_element(2, 3.0f), Vec4(0.0f, 0.0f, 3.0f, 0.0f));
    ASSERT_EQ(Vec4{}.with_element(3, 3.0f), Vec4(0.0f, 0.0f, 0.0f, 3.0f));
}
