#pragma once

#include <oscar/Maths/Vec3.h>

#include <iosfwd>

namespace osc
{
    struct Segment final {
        Vec3 p1{};
        Vec3 p2{};
    };

    std::ostream& operator<<(std::ostream&, Segment const&);
}
