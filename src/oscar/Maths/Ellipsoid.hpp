#pragma once

#include <glm/vec3.hpp>

#include <array>

namespace osc
{
    struct Ellipsoid final {
        glm::vec3 origin;
        glm::vec3 radii;
        std::array<glm::vec3, 3> radiiDirections;
    };
}
