#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace osc {
    struct Textured_vert final {
        glm::vec3 pos;
        glm::vec3 normal;
        glm::vec2 texcoord;
    };
}
