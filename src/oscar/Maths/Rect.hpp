#pragma once

#include <glm/vec2.hpp>

#include <iosfwd>

namespace osc
{
    struct Rect final {
        glm::vec2 p1;
        glm::vec2 p2;

        friend bool operator==(Rect const&, Rect const&) noexcept;
        friend bool operator!=(Rect const&, Rect const&) noexcept;
        friend std::ostream& operator<<(std::ostream&, Rect const&);
    };
}
