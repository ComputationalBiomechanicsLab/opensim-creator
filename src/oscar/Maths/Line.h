#pragma once

#include <oscar/Maths/Vec3.h>

#include <iosfwd>

namespace osc
{
    // an infinitely long positioned + oriented line in 3D space
    //
    // - see `LineSegment` for the finite version
    // - sometimes called `Ray` in the literature
    struct Line final {
        Vec3 origin{};
        Vec3 direction = {0.0f, 1.0f, 0.0f};
    };

    std::ostream& operator<<(std::ostream&, Line const&);
}
