#pragma once

#include <glm/vec3.hpp>

#include <iosfwd>

namespace osc
{
    struct Plane final {
        glm::vec3 origin;
        glm::vec3 normal;
    };

    std::ostream& operator<<(std::ostream&, Plane const&);
}
