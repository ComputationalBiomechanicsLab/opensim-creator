#pragma once

#include <oscar/Graphics/Mesh.h>
#include <oscar/Maths/Angle.h>
#include <oscar/Maths/Vec2.h>

#include <cstddef>
#include <span>
#include <vector>

namespace osc
{
    // returns a mesh with axial symmetry like vases. The lathe rotates around the Y axis.
    //
    // (ported from three.js:LatheGeometry)
    class LatheGeometry final {
    public:
        LatheGeometry(
            std::span<const Vec2> points = std::vector<Vec2>{{0.0f, -0.5f}, {0.5f, 0.0f}, {0.0f, 0.5f}},
            size_t segments = 12,
            Radians phiStart = Degrees{0},
            Radians phiLength = Degrees{360}
        );

        const Mesh& mesh() const { return m_Mesh; }
        operator const Mesh& () const { return m_Mesh; }
    private:
        Mesh m_Mesh;
    };
}
