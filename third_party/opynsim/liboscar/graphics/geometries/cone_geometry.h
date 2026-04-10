#pragma once

#include <liboscar/graphics/mesh.h>
#include <liboscar/maths/angle.h>
#include <liboscar/utilities/c_string_view.h>

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

    class ConeGeometry final {
    public:
        using Params = ConeGeometryParams;

        static constexpr CStringView name() { return "Cone"; }

        explicit ConeGeometry(const Params& = {});

        const Mesh& mesh() const { return mesh_; }
        operator const Mesh& () const { return mesh_; }
    private:
        Mesh mesh_;
    };
}
