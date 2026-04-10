#pragma once

#include <liboscar/graphics/mesh.h>
#include <liboscar/maths/angle.h>
#include <liboscar/utilities/c_string_view.h>

#include <cstddef>

namespace osc
{
    struct CircleGeometryParams final {
        friend bool operator==(const CircleGeometryParams&, const CircleGeometryParams&) = default;

        float radius = 1.0f;
        size_t num_segments = 32;
        Radians theta_start = Degrees{0};
        Radians theta_length = Degrees{360};
    };

    class CircleGeometry final {
    public:
        using Params = CircleGeometryParams;

        static constexpr CStringView name() { return "Circle"; }

        explicit CircleGeometry(const Params& = Params{});

        const Mesh& mesh() const { return mesh_; }
        operator const Mesh& () const { return mesh_; }
    private:
        Mesh mesh_;
    };
}
