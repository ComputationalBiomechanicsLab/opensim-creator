#pragma once

#include <oscar/Graphics/Mesh.h>
#include <oscar/Maths/Angle.h>

#include <cstddef>

namespace osc
{
    // (ported from three.js/TorusGeometry)
    class TorusGeometry final {
    public:
        TorusGeometry(
            float radius = 1.0f,
            float tube = 0.4f,
            size_t radialSegments = 12,
            size_t tubularSegments = 48,
            Radians arc = Degrees{360}
        );

        const Mesh& mesh() const { return m_Mesh; }
        operator const Mesh& () const { return m_Mesh; }
    private:
        Mesh m_Mesh;
    };
}
