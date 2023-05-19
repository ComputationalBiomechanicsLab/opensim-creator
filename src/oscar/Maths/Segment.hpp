#pragma once

#include <glm/vec3.hpp>

#include <iosfwd>

namespace osc
{
    struct Segment final {
        glm::vec3 p1;
        glm::vec3 p2;
    };

    std::ostream& operator<<(std::ostream&, Segment const&);
}
