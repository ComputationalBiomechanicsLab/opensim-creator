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
            size_t num_segments = 12,
            Radians phi_start = Degrees{0},
            Radians phi_length = Degrees{360}
        );

        const Mesh& mesh() const { return mesh_; }
        operator const Mesh& () const { return mesh_; }
    private:
        Mesh mesh_;
    };
}
