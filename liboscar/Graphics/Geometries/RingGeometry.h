#pragma once

#include <liboscar/Graphics/Mesh.h>
#include <liboscar/Maths/Angle.h>
#include <liboscar/Utils/CStringView.h>

#include <cstddef>

namespace osc
{
    struct RingGeometryParams final {
        friend bool operator==(const RingGeometryParams&, const RingGeometryParams&) = default;

        float inner_radius = 0.5f;
        float outer_radius = 1.0f;
        size_t num_theta_segments = 32;
        size_t num_phi_segments = 1;
        Radians theta_start = Degrees{0};
        Radians theta_length = Degrees{360};
    };

    class RingGeometry final : public Mesh {
    public:
        using Params = RingGeometryParams;

        static constexpr CStringView name() { return "Ring"; }

        explicit RingGeometry(const Params& = {});
    };
}
