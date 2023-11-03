#pragma once

#include <oscar/Maths/Vec3.hpp>

#include <iosfwd>

namespace osc
{
    struct Disc final {
        Vec3 origin;
        Vec3 normal;
        float radius;
    };

    std::ostream& operator<<(std::ostream&, Disc const&);
}
