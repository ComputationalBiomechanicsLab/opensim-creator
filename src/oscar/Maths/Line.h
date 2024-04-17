#pragma once

#include <oscar/Maths/Vec3.h>

#include <iosfwd>

namespace osc
{
    // an infinitely long object with no width with an origin location and direction in 3D space
    //
    // - see `LineSegment` for the finite version of this
    // - sometimes called `Ray` in the literature
    struct Line final {
        Vec3 origin{};
        Vec3 direction = {0.0f, 1.0f, 0.0f};
    };

    std::ostream& operator<<(std::ostream&, const Line&);
}
