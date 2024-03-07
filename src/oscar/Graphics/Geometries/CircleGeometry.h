#pragma once

#include <oscar/Graphics/Mesh.h>
#include <oscar/Maths/Angle.h>

#include <cstddef>

namespace osc
{
    // returns a mesh representation of a solid circle
    //
    // (ported from three.js:CircleGeometry)
    class CircleGeometry final {
    public:
        CircleGeometry(
            float radius = 1.0f,
            size_t segments = 32,
            Radians thetaStart = Degrees{0},
            Radians thetaLength = Degrees{360}
        );

        Mesh const& mesh() const { return m_Mesh; }
        operator Mesh const& () const { return m_Mesh; }
    private:
        Mesh m_Mesh;
    };
}
