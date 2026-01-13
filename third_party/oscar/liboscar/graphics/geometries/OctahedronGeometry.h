#pragma once

#include <liboscar/graphics/Mesh.h>
#include <liboscar/utils/CStringView.h>

#include <cstddef>

namespace osc
{
    struct OctahedronGeometryParams final {
        friend bool operator==(const OctahedronGeometryParams&, const OctahedronGeometryParams&) = default;

        float radius = 1.0f;
        size_t detail = 0;
    };

    class OctahedronGeometry final {
    public:
        using Params = OctahedronGeometryParams;

        static constexpr CStringView name() { return "Octahedron"; }

        explicit OctahedronGeometry(const Params& = {});

        const Mesh& mesh() const { return mesh_; }
        operator const Mesh& () const { return mesh_; }
    private:
        Mesh mesh_;
    };
}
