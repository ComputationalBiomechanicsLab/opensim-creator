#pragma once

#include <liboscar/Maths/Vec3.h>

#include <iosfwd>

namespace osc
{
    // Represents a 3D axis-aligned bounding box in a caller-defined
    // coordinate system.
    //
    // The 1D equivalent to an `AABB` is a `ClosedInterval`. The 2D equivalent
    // is a `Rect`.
    struct AABB final {

        friend bool operator==(const AABB&, const AABB&) = default;

        Vec3 min{};
        Vec3 max{};
    };

    std::ostream& operator<<(std::ostream&, const AABB&);
}
