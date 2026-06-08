#include "constants.h"

#include <liboscar/maths/angle.h>

#include <gtest/gtest.h>

using namespace osc;

TEST(deg_to_rad_v, produces_same_value_as_Angle)
{
    ASSERT_NEAR(90.0f * deg_to_rad_v<float>,  Radians {Degrees {90.0f}}.count(), 0.000001f);
    ASSERT_NEAR(90.0  * deg_to_rad_v<double>, Radiansd{Degreesd{90.0 }}.count(), 0.0000000000000001);
}

TEST(rad_to_deg_v, produces_same_value_as_Angle)
{
    ASSERT_NEAR(2.0f * rad_to_deg_v<float>,  Degrees {Radians {2.0f}}.count(), 0.00001f);
    ASSERT_NEAR(2.0  * rad_to_deg_v<double>, Degreesd{Radiansd{2.0 }}.count(), 0.0000000000000001);
}
