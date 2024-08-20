#pragma once

#include <oscar/Graphics/Mesh.h>
#include <oscar/Maths/Angle.h>
#include <oscar/Utils/CStringView.h>

#include <cstddef>

namespace osc
{
    struct SphereGeometryParams final {
        friend bool operator==(const SphereGeometryParams&, const SphereGeometryParams&) = default;

        float radius = 1.0f;
        size_t num_width_segments = 32;
        size_t num_height_segments = 16;
        Radians phi_start = Degrees{0};
        Radians phi_length = Degrees{360};
        Radians theta_start = Degrees{0};
        Radians theta_length = Degrees{180};
    };

    class SphereGeometry final : public Mesh {
    public:
        using Params = SphereGeometryParams;

        static constexpr CStringView name() { return "Sphere"; }

        explicit SphereGeometry(const Params& = {});
    };
}
