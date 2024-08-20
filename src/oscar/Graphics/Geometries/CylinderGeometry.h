#pragma once

#include <oscar/Graphics/Mesh.h>
#include <oscar/Maths/Angle.h>
#include <oscar/Utils/CStringView.h>

#include <cstddef>

namespace osc
{
    struct CylinderGeometryParams final {
        friend bool operator==(const CylinderGeometryParams&, const CylinderGeometryParams&) = default;

        float radius_top = 1.0f;
        float radius_bottom = 1.0f;
        float height = 1.0f;
        size_t num_radial_segments = 32;
        size_t num_height_segments = 1;
        bool open_ended = false;
        Radians theta_start = Degrees{0};
        Radians theta_length = Degrees{360};
    };

    class CylinderGeometry final : public Mesh {
    public:
        using Params = CylinderGeometryParams;

        static constexpr CStringView name() { return "Cylinder"; }

        explicit CylinderGeometry(const Params& = {});
    };
}
