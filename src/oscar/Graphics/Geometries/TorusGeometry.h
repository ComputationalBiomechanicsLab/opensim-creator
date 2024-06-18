#pragma once

#include <oscar/Graphics/Mesh.h>
#include <oscar/Maths/Angle.h>
#include <oscar/Utils/CStringView.h>

#include <cstddef>

namespace osc
{
    class TorusGeometry final {
    public:
        static constexpr CStringView name() { return "Torus"; }

        TorusGeometry(
            float tube_center_radius = 1.0f,
            float tube_radius = 0.4f,
            size_t num_radial_segments = 12,
            size_t num_tubular_segments = 48,
            Radians arc = Degrees{360}
        );

        const Mesh& mesh() const { return mesh_; }
        operator const Mesh& () const { return mesh_; }
    private:
        Mesh mesh_;
    };
}
