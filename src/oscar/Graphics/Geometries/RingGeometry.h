#pragma once

#include <oscar/Graphics/Mesh.h>
#include <oscar/Maths/Angle.h>

#include <cstddef>

namespace osc
{
    class RingGeometry final {
    public:
        RingGeometry(
            float inner_radius = 0.5f,
            float outer_radius = 1.0f,
            size_t num_theta_segments = 32,
            size_t num_phi_segments = 1,
            Radians theta_start = Degrees{0},
            Radians theta_length = Degrees{360}
        );

        const Mesh& mesh() const { return mesh_; }
        operator const Mesh& () const { return mesh_; }
    private:
        Mesh mesh_;
    };
}
