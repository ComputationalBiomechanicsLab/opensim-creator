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

        Mesh const& mesh() const { return m_Mesh; }
        operator Mesh const& () const { return m_Mesh; }
    private:
        Mesh m_Mesh;
    };
}
