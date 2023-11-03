#pragma once

#include <oscar/Maths/Vec3.hpp>

#include <iosfwd>

namespace osc
{
    struct Line final {
        Vec3 origin;
        Vec3 dir;
    };

    std::ostream& operator<<(std::ostream&, Line const&);
}
