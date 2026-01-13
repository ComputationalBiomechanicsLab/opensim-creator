#include "Transform.h"

#include <liboscar/maths/Quaternion.h>
#include <liboscar/maths/QuaternionFunctions.h>

#include <gtest/gtest.h>

using namespace osc;

TEST(Transform, default_constructed_has_an_identity_rotation)
{
    // sanity check: the transform ctor depends on this
    static_assert(Transform{}.rotation == identity<Quaternion>());
}
