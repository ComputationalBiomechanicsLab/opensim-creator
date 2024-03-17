#pragma once

#include <oscar/Maths/Vec3.h>
#include <oscar/Utils/HashHelpers.h>

#include <cstddef>
#include <functional>
#include <iosfwd>

namespace osc
{
    // a finite-length line delimited by two endpoints in 3D space
    struct LineSegment final {

        constexpr friend bool operator==(LineSegment const&, LineSegment const&) = default;

        Vec3 start{};
        Vec3 end{};
    };

    std::ostream& operator<<(std::ostream&, LineSegment const&);
}

template<>
struct std::hash<osc::LineSegment> {
    size_t operator()(osc::LineSegment const& ls) const
    {
        return osc::HashOf(ls.start, ls.end);
    }
};
