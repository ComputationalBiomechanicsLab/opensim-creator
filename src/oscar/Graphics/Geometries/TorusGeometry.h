#pragma once

#include <oscar/Graphics/Mesh.h>
#include <oscar/Maths/Angle.h>
#include <oscar/Utils/CStringView.h>

#include <cstddef>

namespace osc
{
    struct TorusGeometryParams final {
        friend bool operator==(const TorusGeometryParams&, const TorusGeometryParams&) = default;

        float tube_center_radius = 1.0f;
        float tube_radius = 0.4f;
        size_t num_radial_segments = 12;
        size_t num_tubular_segments = 48;
        Radians arc = Degrees{360};
    };

    class TorusGeometry final : public Mesh {
    public:
        using Params = TorusGeometryParams;

        static constexpr CStringView name() { return "Torus"; }

        explicit TorusGeometry(const Params& = {});
    };
}
