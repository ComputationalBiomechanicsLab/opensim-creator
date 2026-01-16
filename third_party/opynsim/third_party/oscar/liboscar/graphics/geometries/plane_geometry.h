#pragma once

#include <liboscar/graphics/mesh.h>
#include <liboscar/maths/vector2.h>
#include <liboscar/maths/vector3.h>
#include <liboscar/utils/c_string_view.h>

namespace osc
{
    struct PlaneGeometryParams final {
        friend bool operator==(const PlaneGeometryParams&, const PlaneGeometryParams&) = default;

        Vector2 dimensions = {1.0f, 1.0f};
        Vector2uz num_segments = {1, 1};
    };

    class PlaneGeometry final {
    public:
        using Params = PlaneGeometryParams;

        static constexpr CStringView name() { return "Plane"; }

        explicit PlaneGeometry(const Params& = {});

        Vector3 normal() const { return {0.0f, 0.0f, 1.0f}; }

        const Mesh& mesh() const { return mesh_; }
        operator const Mesh& () const { return mesh_; }
    private:
        Mesh mesh_;
    };
}
