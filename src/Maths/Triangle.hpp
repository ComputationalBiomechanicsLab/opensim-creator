#pragma once

#include <glm/vec3.hpp>

namespace osc
{
    struct Triangle final {
        glm::vec3 const& operator[](size_t i) const noexcept
        {
            static_assert(alignof(Triangle) == alignof(glm::vec3));
            static_assert(sizeof(Triangle) == 3*sizeof(glm::vec3));
            return reinterpret_cast<glm::vec3 const*>(this)[i];
        }

        glm::vec3 p0;
        glm::vec3 p1;
        glm::vec3 p2;
    };
}