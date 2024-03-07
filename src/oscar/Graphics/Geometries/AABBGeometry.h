#pragma once

#include <oscar/Graphics/Mesh.h>
#include <oscar/Maths/AABB.h>

namespace osc
{
    class AABBGeometry final {
    public:
        AABBGeometry(AABB const& = {.min = {-1.0f, -1.0f, -1.0f}, .max = {1.0f, 1.0f, 1.0f}});

        Mesh const& mesh() const { return m_Mesh; }
        operator Mesh const& () const { return m_Mesh; }
    private:
        Mesh m_Mesh;
    };
}
