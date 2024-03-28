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
            size_t num_segments = 32,
            Radians theta_start = Degrees{0},
            Radians theta_length = Degrees{360}
        );

        const Mesh& mesh() const { return mesh_; }
        operator const Mesh& () const { return mesh_; }
    private:
        Mesh mesh_;
    };
}
