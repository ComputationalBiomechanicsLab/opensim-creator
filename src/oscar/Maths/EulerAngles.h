#pragma once

#include <oscar/Maths/Angle.h>
#include <oscar/Maths/Vec.h>

namespace osc
{
    // a sequence of X -> Y -> Z euler angles
    using EulerAngles = Vec<3, Radians>;
}
