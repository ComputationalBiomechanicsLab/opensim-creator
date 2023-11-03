#pragma once

#include <oscar/Maths/Vec3.hpp>

#include <iosfwd>

namespace osc
{
    struct Sphere final {
        Vec3 origin = {};
        float radius = 1.0f;
    };

    std::ostream& operator<<(std::ostream&, Sphere const&);
}
