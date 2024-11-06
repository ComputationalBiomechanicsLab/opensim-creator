#include <oscar/Maths/Transform.h>

#include <oscar/Maths/Quat.h>
#include <oscar/Maths/QuaternionFunctions.h>

#include <gtest/gtest.h>

using namespace osc;

TEST(Transform, default_constructed_has_an_identity_rotation)
{
    // sanity check: the transform ctor depends on this
    static_assert(Transform{}.rotation == identity<Quat>());
}
