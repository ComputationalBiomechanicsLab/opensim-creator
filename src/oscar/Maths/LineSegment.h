#pragma once

#include <oscar/Maths/Vec3.h>
#include <oscar/Utils/HashHelpers.h>

#include <cstddef>
#include <functional>
#include <iosfwd>

namespace osc
{
    struct LineSegment final {
        Vec3 p1{};
        Vec3 p2{};

        constexpr friend bool operator==(LineSegment const&, LineSegment const&) = default;
    };

    std::ostream& operator<<(std::ostream&, LineSegment const&);
}

template<>
struct std::hash<osc::LineSegment> {
    size_t operator()(osc::LineSegment const& ls) const
    {
        return osc::HashOf(ls.p1, ls.p2);
    }
};
