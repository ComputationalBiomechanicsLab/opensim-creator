#pragma once

#include <glm/vec3.hpp>

namespace osc
{
    struct Triangle final {
        glm::vec3 p0;
        glm::vec3 p1;
        glm::vec3 p2;
    };
}