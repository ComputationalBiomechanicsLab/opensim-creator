#pragma once

#include <oscar/Maths/Vec2.h>

#include <iosfwd>

namespace osc
{
    struct Rect final {

        friend bool operator==(Rect const&, Rect const&) = default;

        Vec2 p1{};
        Vec2 p2{};
    };

    std::ostream& operator<<(std::ostream&, Rect const&);
}
