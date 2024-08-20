#pragma once

#include <oscar/Graphics/Mesh.h>
#include <oscar/Maths/Angle.h>
#include <oscar/Utils/CStringView.h>

#include <cstddef>

namespace osc
{
    struct ConeGeometryParams final {
        friend bool operator==(const ConeGeometryParams&, const ConeGeometryParams&) = default;

        float radius = 1.0f;
        float height = 1.0f;
        size_t num_radial_segments = 32;
        size_t num_height_segments = 1;
        bool open_ended = false;
        Radians theta_start = Degrees{0};
        Radians theta_length = Degrees{360};
    };

    class ConeGeometry final : public Mesh {
    public:
        using Params = ConeGeometryParams;

        static constexpr CStringView name() { return "Cone"; }

        explicit ConeGeometry(const Params& = {});
    };
}
