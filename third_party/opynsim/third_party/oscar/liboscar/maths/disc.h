#pragma once

#include <liboscar/maths/vector3.h>

#include <iosfwd>

namespace osc
{
    // a region in a 3D plane bounded by a circle
    //
    // - see `Circle` for the 2D equivalent of this
    struct Disc final {
        friend bool operator==(const Disc&, const Disc&) = default;

        Vector3 origin{};
        Vector3 normal = {0.0f, 1.0f, 0.0f};
        float radius = 1.0f;
    };

    std::ostream& operator<<(std::ostream&, const Disc&);
}
