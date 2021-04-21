#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace osc {
    struct Untextured_vert final {
        glm::vec3 pos;
        glm::vec3 normal;

        Untextured_vert() = default;

        constexpr Untextured_vert(glm::vec3 _pos, glm::vec3 _normal) noexcept : pos{_pos}, normal{_normal} {
        }
    };
}
