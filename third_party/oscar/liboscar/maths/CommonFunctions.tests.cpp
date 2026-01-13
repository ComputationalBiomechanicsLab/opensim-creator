#include "CommonFunctions.h"

#include <liboscar/maths/Angle.h>
#include <liboscar/maths/Vector.h>
#include <liboscar/maths/Vector3.h>

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

TEST(abs, works_on_Vector3)
{
    ASSERT_EQ(abs(Vector3(-1.0f, -2.0f, 3.0f)), Vector3(1.0f, 2.0f, 3.0f));
}

TEST(abs, works_on_Vector3i)
{
    ASSERT_EQ(abs(Vector3i(-3, -2, 0)), Vector3i(3, 2, 0));
}

TEST(mod, works_on_Vector3_of_Angles)
{
    constexpr Vector<Degrees, 3> x{10.0f, 3.0f, 4.0f};
    constexpr Vector<Degrees, 3> y{8.0f, 2.0f, 1.0f};
    constexpr Vector<Degrees, 3> expected{2.0f, 1.0f, 0.0f};

    ASSERT_EQ(mod(x, y), expected);
}
