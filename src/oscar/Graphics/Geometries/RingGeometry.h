#pragma once

#include <oscar/Graphics/Mesh.h>
#include <oscar/Maths/Angle.h>
#include <oscar/Utils/CStringView.h>

#include <cstddef>

namespace osc
{
    class RingGeometry final : public Mesh {
    public:
        static constexpr CStringView name() { return "Ring"; }

        struct Params final {
            float inner_radius = 0.5f;
            float outer_radius = 1.0f;
            size_t num_theta_segments = 32;
            size_t num_phi_segments = 1;
            Radians theta_start = Degrees{0};
            Radians theta_length = Degrees{360};
        };

        explicit RingGeometry(const Params& = {});
    };
}
