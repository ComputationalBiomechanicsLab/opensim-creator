#pragma once

#include <liboscar/graphics/Mesh.h>
#include <liboscar/utils/CStringView.h>

#include <cstddef>

namespace osc
{
    struct TetrahedronGeometryParams final {
        friend bool operator==(const TetrahedronGeometryParams&, const TetrahedronGeometryParams&) = default;

        float radius = 1.0f;
        size_t detail_level = 0;
    };

    class TetrahedronGeometry final {
    public:
        using Params = TetrahedronGeometryParams;

        static constexpr CStringView name() { return "Tetrahedron"; }

        explicit TetrahedronGeometry(const Params& = {});

        const Mesh& mesh() const { return mesh_; }
        operator const Mesh& () const { return mesh_; }
    private:
        Mesh mesh_;
    };
}
