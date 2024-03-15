#pragma once

#include <oscar/Maths/Vec3.h>

#include <iosfwd>

namespace osc
{
    // a planar disc in 3D space
    //
    // - see `Circle` for the 2D equivalent of this
    struct Disc final {
        Vec3 origin{};
        Vec3 normal = {0.0f, 1.0f, 0.0f};
        float radius = 1.0f;
    };

    std::ostream& operator<<(std::ostream&, Disc const&);
}
