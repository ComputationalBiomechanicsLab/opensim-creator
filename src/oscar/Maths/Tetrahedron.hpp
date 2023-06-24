#pragma once

#include <glm/vec3.hpp>

#include <array>

namespace osc
{
    struct Tetrahedron final {

        glm::vec3 const* begin() const
        {
            return verts.data();
        }

        glm::vec3 const* end() const
        {
            return verts.data() + verts.size();
        }

        glm::vec3 const& operator[](size_t i) const
        {
            return verts[i];
        }

        glm::vec3& operator[](size_t i)
        {
            return verts[i];
        }

        constexpr size_t size() const
        {
            return 4;
        }

        std::array<glm::vec3, 4> verts;
    };

    float Volume(Tetrahedron const&);
    glm::vec3 Center(Tetrahedron const&);
}