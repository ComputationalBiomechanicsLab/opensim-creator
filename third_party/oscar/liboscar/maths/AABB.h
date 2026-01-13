#pragma once

#include <liboscar/maths/Vector3.h>

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

        Vector3 min{};
        Vector3 max{};
    };

    std::ostream& operator<<(std::ostream&, const AABB&);
}
