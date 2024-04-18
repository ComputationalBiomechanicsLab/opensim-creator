#pragma once

#include <oscar/Maths/UnitVec.h>
#include <oscar/Maths/Vec3.h>

namespace osc
{
    using UnitVec3 = UnitVec<3, float>;
    using UnitVec3f = UnitVec<3, float>;
    using UnitVec3d = UnitVec<3, double>;

    // returns the cross product of `x` and `y`
    //
    // (specialization: this implementation "knows" the product should already be normalized)
    template<std::floating_point T>
    constexpr UnitVec<3, T> cross(const UnitVec<3, T>& x, const UnitVec<3, T>& y)
    {
        return UnitVec<3, T>::already_normalized(
            x[1] * y[2] - y[1] * x[2],
            x[2] * y[0] - y[2] * x[0],
            x[0] * y[1] - y[0] * x[1]
        );
    }
}
