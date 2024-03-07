#pragma once

#include <oscar/Graphics/Mesh.h>

#include <cstddef>

namespace osc
{
    class GridGeometry final {
    public:
        GridGeometry(
            float size = 2.0f,
            size_t divisions = 10
        );

        Mesh const& mesh() const { return m_Mesh; }
        operator Mesh const& () const { return m_Mesh; }
    private:
        Mesh m_Mesh;
    };
}
