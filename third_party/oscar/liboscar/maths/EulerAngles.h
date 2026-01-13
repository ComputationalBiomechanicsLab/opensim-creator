#pragma once

#include <liboscar/maths/Angle.h>
#include <liboscar/maths/Vector.h>

namespace osc
{
    // in OSC, Euler angles represent an intrinsic 3D rotation about
    // the X, then Y, then Z axes

    template<typename Units>
    using EulerAnglesIn = Vector<Units, 3>;  // useful for writing `EulerAnglesIn<Degrees>(vec)`

    using EulerAngles = EulerAnglesIn<Radians>;
}
