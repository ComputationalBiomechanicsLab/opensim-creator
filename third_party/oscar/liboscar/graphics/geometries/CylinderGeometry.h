#pragma once

#include <liboscar/graphics/Mesh.h>
#include <liboscar/maths/Angle.h>
#include <liboscar/utils/CStringView.h>

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

    class CylinderGeometry final {
    public:
        using Params = CylinderGeometryParams;

        static constexpr CStringView name() { return "Cylinder"; }

        explicit CylinderGeometry(const Params& = {});

        const Mesh& mesh() const { return mesh_; }
        operator const Mesh& () const { return mesh_; }
    private:
        Mesh mesh_;
    };
}
