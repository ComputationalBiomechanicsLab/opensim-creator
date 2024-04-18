#pragma once

#include <oscar/Maths/AnalyticPlane.h>
#include <oscar/Maths/Vec3.h>

#include <iosfwd>

namespace osc
{
    // a plane expressed in point-normal form
    //
    // - useful when you want to control where a limited-size plane should
    //   actually be drawn in 3D
    //
    // - see `AnalyticPlane` for a struct that's better in maths functions, because
    //   it represents the general equation of a plane (i.e. ax + by + cz + d = 0)
    struct Plane final {
        Vec3 origin{};
        Vec3 normal{0.0f, 1.0f, 0.0f};
    };

    std::ostream& operator<<(std::ostream&, const Plane&);
}
