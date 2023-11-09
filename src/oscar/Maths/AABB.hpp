#pragma once

#include <oscar/Maths/Vec3.hpp>

#include <iosfwd>

namespace osc
{
    struct AABB final {

        constexpr AABB() = default;

        explicit constexpr AABB(Vec3 const& v) :
            min{v},
            max{v}
        {
        }

        constexpr AABB(
            Vec3 const& min_,
            Vec3 const& max_
        ) :
            min{min_},
            max{max_}
        {
        }

        friend bool operator==(AABB const&, AABB const&) = default;

        Vec3 min{};
        Vec3 max{};
    };

    std::ostream& operator<<(std::ostream&, AABB const&);
}
