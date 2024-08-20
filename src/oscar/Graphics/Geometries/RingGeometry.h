#pragma once

#include <oscar/Graphics/Mesh.h>
#include <oscar/Maths/Angle.h>
#include <oscar/Utils/CStringView.h>

#include <cstddef>

namespace osc
{
    struct RingGeometryParams final {
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
