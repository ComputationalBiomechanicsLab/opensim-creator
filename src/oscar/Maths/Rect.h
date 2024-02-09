#pragma once

#include <oscar/Maths/Vec2.h>

#include <iosfwd>

namespace osc
{
    struct Rect final {
        Vec2 p1{};
        Vec2 p2{};

        friend bool operator==(Rect const&, Rect const&) = default;
    };
    std::ostream& operator<<(std::ostream&, Rect const&);
}
