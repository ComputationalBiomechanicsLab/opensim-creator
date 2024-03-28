#pragma once

#include <oscar/Graphics/Mesh.h>
#include <oscar/Maths/Angle.h>

#include <cstddef>

namespace osc
{
    class SphereGeometry final {
    public:
        SphereGeometry(
            float radius = 1.0f,
            size_t num_width_segments = 32,
            size_t num_height_segments = 16,
            Radians phi_start = Degrees{0},
            Radians phi_length = Degrees{360},
            Radians theta_start = Degrees{0},
            Radians theta_length = Degrees{180}
        );

        const Mesh& mesh() const { return mesh_; }
        operator const Mesh& () const { return mesh_; }
    private:
        Mesh mesh_;
    };
}
