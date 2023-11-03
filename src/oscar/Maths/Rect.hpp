#pragma once

#include <oscar/Maths/Vec2.hpp>

#include <iosfwd>

namespace osc
{
    struct Rect final {
        Vec2 p1;
        Vec2 p2;
    };
    bool operator==(Rect const&, Rect const&) noexcept;
    bool operator!=(Rect const&, Rect const&) noexcept;
    std::ostream& operator<<(std::ostream&, Rect const&);
}
