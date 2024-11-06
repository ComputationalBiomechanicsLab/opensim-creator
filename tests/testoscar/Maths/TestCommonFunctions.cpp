#include <oscar/Maths/CommonFunctions.h>

#include <oscar/Maths/Angle.h>
#include <oscar/Maths/Vec.h>
#include <oscar/Maths/Vec3.h>

#include <gtest/gtest.h>

using namespace osc;

TEST(abs, works_on_floating_point_numbers)
{
    ASSERT_EQ(abs(-1.4f), 1.4f);
}

TEST(abs, works_on_signed_integers)
{
    ASSERT_EQ(abs(-5), 5);
}

TEST(abs, works_on_Vec3)
{
    ASSERT_EQ(abs(Vec3(-1.0f, -2.0f, 3.0f)), Vec3(1.0f, 2.0f, 3.0f));
}

TEST(abs, works_on_Vec3i)
{
    ASSERT_EQ(abs(Vec3i(-3, -2, 0)), Vec3i(3, 2, 0));
}

TEST(mod, works_on_Vec3_of_Angles)
{
    const Vec<3, Degrees> x{10.0f, 3.0f, 4.0f};
    const Vec<3, Degrees> y{8.0f, 2.0f, 1.0f};
    const Vec<3, Degrees> expected{2.0f, 1.0f, 0.0f};

    ASSERT_EQ(mod(x, y), expected);
}
