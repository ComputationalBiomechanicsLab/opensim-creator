#pragma once

#include <oscar/Maths/Vec3.hpp>

#include <iosfwd>

namespace osc
{
    struct AABB final {

        constexpr static AABB OfPoint(Vec3 const& p)
        {
            return AABB{p, p};
        }

        friend bool operator==(AABB const&, AABB const&) = default;

        Vec3 min{};
        Vec3 max{};
    };

    std::ostream& operator<<(std::ostream&, AABB const&);
}
