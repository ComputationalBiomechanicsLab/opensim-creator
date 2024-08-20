#pragma once

#include <oscar/Graphics/Mesh.h>
#include <oscar/Maths/Angle.h>
#include <oscar/Utils/CStringView.h>

#include <cstddef>

namespace osc
{
    struct CircleGeometryParams final {
        float radius = 1.0f;
        size_t num_segments = 32;
        Radians theta_start = Degrees{0};
        Radians theta_length = Degrees{360};
    };

    class CircleGeometry final : public Mesh {
    public:
        using Params = CircleGeometryParams;

        static constexpr CStringView name() { return "Circle"; }

        explicit CircleGeometry(const Params& = Params{});
    };
}
