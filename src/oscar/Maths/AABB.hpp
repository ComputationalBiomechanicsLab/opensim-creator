#pragma once

#include <oscar/Maths/Vec3.hpp>

#include <iosfwd>

namespace osc
{
    struct AABB final {
        Vec3 min;
        Vec3 max;
    };

    bool operator==(AABB const&, AABB const&) noexcept;
    bool operator!=(AABB const&, AABB const&) noexcept;
    std::ostream& operator<<(std::ostream&, AABB const&);
}
