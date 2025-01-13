#pragma once

#include <liboscar/Maths/Angle.h>
#include <liboscar/Maths/Vec.h>

namespace osc
{
    // in OSC, Euler angles represent an intrinsic 3D rotation about
    // the X, then Y, then Z axes

    template<typename Units>
    using EulerAnglesIn = Vec<3, Units>;  // useful for writing `EulerAnglesIn<Degrees>(vec)`

    using EulerAngles = EulerAnglesIn<Radians>;
}
