#pragma once

#include <oscar/Maths/Vec3.h>

#include <iosfwd>

namespace osc
{
    // an axis-aligned bounding box (AABB) in 3D space
    struct AABB final {

        friend bool operator==(const AABB&, const AABB&) = default;

        Vec3 min{};
        Vec3 max{};
    };

    std::ostream& operator<<(std::ostream&, const AABB&);
}
