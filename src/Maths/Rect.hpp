#pragma once

#include <glm/vec2.hpp>

#include <iosfwd>

namespace osc
{
    struct Rect final {
        glm::vec2 p1;
        glm::vec2 p2;
    };

    std::ostream& operator<<(std::ostream&, Rect const&);
}
