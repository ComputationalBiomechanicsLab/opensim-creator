#pragma once

#include <oscar/Graphics/Mesh.h>

#include <cstddef>

namespace osc
{
    // (ported from three.js/PlaneGeometry)
    class PlaneGeometry final {
    public:
        PlaneGeometry(
            float width = 1.0f,
            float height = 1.0f,
            size_t widthSegments = 1,
            size_t heightSegments = 1
        );

        const Mesh& mesh() const { return m_Mesh; }
        operator const Mesh& () const { return m_Mesh; }
    private:
        Mesh m_Mesh;
    };
}
