#pragma once

#include <liboscar/graphics/mesh.h>
#include <liboscar/utilities/c_string_view.h>

#include <cstddef>

namespace osc
{
    struct DodecahedronGeometryParams final {
        friend bool operator==(const DodecahedronGeometryParams&, const DodecahedronGeometryParams&) = default;

        float radius = 1.0f;
        size_t detail = 0;
    };

    class DodecahedronGeometry final {
    public:
        using Params = DodecahedronGeometryParams;

        static constexpr CStringView name() { return "Dodecahedron"; }

        explicit DodecahedronGeometry(const Params& = {});

        const Mesh& mesh() const { return mesh_; }
        operator const Mesh& () const { return mesh_; }
    private:
        Mesh mesh_;
    };
}
