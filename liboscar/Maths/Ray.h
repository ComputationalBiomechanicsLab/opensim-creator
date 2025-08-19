#pragma once

#include <liboscar/Maths/Vector3.h>

#include <iosfwd>

namespace osc
{
    // an infinitely long object with no width with an origin position and direction in 3D space
    //
    // - see `LineSegment` for the finite version of this
    // - sometimes called `Ray` in the literature
    struct Ray final {
        Vector3 origin{};
        Vector3 direction = {0.0f, 1.0f, 0.0f};
    };

    std::ostream& operator<<(std::ostream&, const Ray&);
}
