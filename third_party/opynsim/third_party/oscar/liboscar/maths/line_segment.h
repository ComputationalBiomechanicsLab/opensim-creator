#pragma once

#include <liboscar/maths/vector3.h>
#include <liboscar/utilities/hash_helpers.h>

#include <cstddef>
#include <functional>
#include <iosfwd>

namespace osc
{
    // a finite-length line delimited by two endpoints in 3D space
    struct LineSegment final {

        constexpr friend bool operator==(const LineSegment&, const LineSegment&) = default;

        Vector3 start{};
        Vector3 end{};
    };

    std::ostream& operator<<(std::ostream&, const LineSegment&);
}

template<>
struct std::hash<osc::LineSegment> {
    size_t operator()(const osc::LineSegment& line_segment) const noexcept
    {
        return osc::hash_of(line_segment.start, line_segment.end);
    }
};
