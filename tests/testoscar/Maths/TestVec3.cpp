#include <oscar/Maths/Vec3.h>

#include <gtest/gtest.h>

using namespace osc;

TEST(Vec3, with_element_works_as_expected)
{
    ASSERT_EQ(Vec3{}.with_element(0, 2.0f), Vec3(2.0f, 0.0f, 0.0f));
    ASSERT_EQ(Vec3(1.0f).with_element(0, 3.0f), Vec3(3.0f, 1.0f, 1.0f));
    ASSERT_EQ(Vec3{}.with_element(1, 3.0f), Vec3(0.0f, 3.0f, 0.0f));
    ASSERT_EQ(Vec3{}.with_element(2, 3.0f), Vec3(0.0f, 0.0f, 3.0f));
}
