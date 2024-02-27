#pragma once

#include <oscar/Maths/Vec3.h>

#include <iosfwd>

namespace osc
{
    struct LineSegment final {
        Vec3 p1{};
        Vec3 p2{};
    };

    std::ostream& operator<<(std::ostream&, LineSegment const&);
}
