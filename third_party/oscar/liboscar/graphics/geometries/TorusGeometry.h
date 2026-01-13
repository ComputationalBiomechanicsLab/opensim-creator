#pragma once

#include <liboscar/graphics/Mesh.h>
#include <liboscar/maths/Angle.h>
#include <liboscar/utils/CStringView.h>

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

    class TorusGeometry final {
    public:
        using Params = TorusGeometryParams;

        static constexpr CStringView name() { return "Torus"; }

        explicit TorusGeometry(const Params& = {});

        const Mesh& mesh() const { return mesh_; }
        operator const Mesh& () const { return mesh_; }
    private:
        Mesh mesh_;
    };
}
