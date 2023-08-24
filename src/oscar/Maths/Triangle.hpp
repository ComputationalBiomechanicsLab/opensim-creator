#pragma once

#include <glm/vec3.hpp>

#include <cstddef>

namespace osc
{
    struct Triangle final {

        glm::vec3 const& operator[](size_t i) const noexcept
        {
            static_assert(sizeof(Triangle) == 3*sizeof(glm::vec3));
            static_assert(offsetof(Triangle, p0) == 0);
            return (&p0)[i];
        }

        glm::vec3 p0;
        glm::vec3 p1;
        glm::vec3 p2;
    };
}
