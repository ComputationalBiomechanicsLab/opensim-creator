#pragma once

#include <glm/vec3.hpp>

#include <array>

namespace osc
{
    struct Ellipsoid final {
        glm::vec3 origin = {};
        glm::vec3 radii = {1.0f, 1.0f, 1.0f};
        std::array<glm::vec3, 3> radiiDirections =
        {
            glm::vec3{1.0f, 0.0f, 0.0f},
            glm::vec3{0.0f, 1.0f, 0.0f},
            glm::vec3{0.0f, 0.0f, 1.0f},
        };
    };
}
