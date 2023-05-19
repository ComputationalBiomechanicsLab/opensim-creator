#pragma once

#include <glm/vec3.hpp>

#include <iosfwd>

namespace osc
{
    struct Disc final {
        glm::vec3 origin;
        glm::vec3 normal;
        float radius;
    };

    std::ostream& operator<<(std::ostream&, Disc const&);
}
