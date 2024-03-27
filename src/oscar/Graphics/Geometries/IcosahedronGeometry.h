#pragma once

#include <oscar/Graphics/Mesh.h>

#include <cstddef>

namespace osc
{
    class IcosahedronGeometry final {
    public:
        IcosahedronGeometry(
            float radius = 1.0f,
            size_t detail = 0
        );

        const Mesh& mesh() const { return m_Mesh; }
        operator const Mesh& () const { return m_Mesh; }
    private:
        Mesh m_Mesh;
    };
}
