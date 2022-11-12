#pragma once

#include <glm/vec3.hpp>

namespace osc
{
    struct Tetrahedron final {

        glm::vec3 const& operator[](size_t i) const
        {
            return Verts[i];
        }

        glm::vec3& operator[](size_t i)
        {
            return Verts[i];
        }

        constexpr size_t size() const
        {
            return 4;
        }

        glm::vec3 Verts[4];
    };

    float Volume(Tetrahedron const&);
    glm::vec3 Center(Tetrahedron const&);
}