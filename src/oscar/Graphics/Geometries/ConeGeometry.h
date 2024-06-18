#pragma once

#include <oscar/Graphics/Mesh.h>
#include <oscar/Maths/Angle.h>
#include <oscar/Utils/CStringView.h>

#include <cstddef>

namespace osc
{
    class ConeGeometry final {
    public:
        static constexpr CStringView name() { return "Cone"; }

        ConeGeometry(
            float radius = 1.0f,
            float height = 1.0f,
            size_t num_radial_segments = 32,
            size_t num_height_segments = 1,
            bool open_ended = false,
            Radians theta_start = Degrees{0},
            Radians theta_length = Degrees{360}
        );

        const Mesh& mesh() const { return mesh_; }
        operator const Mesh& () const { return mesh_; }
    private:
        Mesh mesh_;
    };
}
