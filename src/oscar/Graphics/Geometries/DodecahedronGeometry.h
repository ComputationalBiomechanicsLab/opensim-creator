#pragma once

#include <oscar/Graphics/Mesh.h>

#include <cstddef>

namespace osc
{
    class DodecahedronGeometry final {
    public:
        DodecahedronGeometry(
            float radius = 1.0f,
            size_t detail = 0
        );

        Mesh const& mesh() const { return m_Mesh; }
        operator Mesh const& () const { return m_Mesh; }
    private:
        Mesh m_Mesh;
    };
}
