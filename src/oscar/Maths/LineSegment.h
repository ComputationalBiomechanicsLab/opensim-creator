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

        constexpr friend bool operator==(const LineSegment&, const LineSegment&) = default;

        Vec3 start{};
        Vec3 end{};
    };

    std::ostream& operator<<(std::ostream&, const LineSegment&);
}

template<>
struct std::hash<osc::LineSegment> {
    size_t operator()(const osc::LineSegment& line_segment) const
    {
        return osc::hash_of(line_segment.start, line_segment.end);
    }
};
