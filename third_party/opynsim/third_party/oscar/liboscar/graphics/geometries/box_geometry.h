#pragma once

#include <liboscar/graphics/mesh.h>
#include <liboscar/maths/vector3.h>
#include <liboscar/utilities/c_string_view.h>

namespace osc
{
    struct BoxGeometryParams final {
        friend bool operator==(const BoxGeometryParams&, const BoxGeometryParams&) = default;

        Vector3 dimensions = {1.0f, 1.0f, 1.0f};
        Vector3uz num_segments = {1, 1, 1};
    };

    class BoxGeometry final {
    public:
        using Params = BoxGeometryParams;

        static constexpr CStringView name() { return "Box"; }

        explicit BoxGeometry(const Params& = Params{});

        const Mesh& mesh() const { return mesh_; }
        operator const Mesh& () const { return mesh_; }
    private:
        Mesh mesh_;
    };
}
