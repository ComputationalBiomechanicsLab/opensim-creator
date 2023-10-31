#pragma once

#include <glm/vec3.hpp>

#include <iosfwd>

namespace osc
{
    struct Plane final {
        glm::vec3 origin = {};
        glm::vec3 normal = {0.0f, 1.0f, 0.0f};
    };

    std::ostream& operator<<(std::ostream&, Plane const&);
}
