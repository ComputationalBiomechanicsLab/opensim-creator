#pragma once

#include <oscar/Maths/Vec3.h>

#include <iosfwd>

namespace osc
{
    struct Plane final {
        Vec3 origin{};
        Vec3 normal{0.0f, 1.0f, 0.0f};
    };

    std::ostream& operator<<(std::ostream&, Plane const&);
}
