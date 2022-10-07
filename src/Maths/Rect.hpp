#pragma once

#include <glm/vec2.hpp>

#include <iosfwd>

namespace osc
{
    struct Rect final {
        glm::vec2 p1;
        glm::vec2 p2;
    };

    // prints a human-readable representation of the Rect to the stream
    std::ostream& operator<<(std::ostream&, Rect const&);
}
