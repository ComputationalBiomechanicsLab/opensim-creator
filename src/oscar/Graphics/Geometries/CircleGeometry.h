#pragma once

#include <oscar/Graphics/Mesh.h>
#include <oscar/Maths/Angle.h>
#include <oscar/Utils/CStringView.h>

#include <cstddef>

namespace osc
{
    class CircleGeometry final : public Mesh {
    public:
        static constexpr CStringView name() { return "Circle"; }

        struct Params final {
            float radius = 1.0f;
            size_t num_segments = 32;
            Radians theta_start = Degrees{0};
            Radians theta_length = Degrees{360};
        };

        explicit CircleGeometry(const Params& = Params{});
    };
}
