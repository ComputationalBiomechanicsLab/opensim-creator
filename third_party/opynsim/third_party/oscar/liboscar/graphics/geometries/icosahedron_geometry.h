#pragma once

#include <liboscar/graphics/mesh.h>
#include <liboscar/utils/c_string_view.h>

#include <cstddef>

namespace osc
{
    struct IcosahedronGeometryParams final {
        friend bool operator==(const IcosahedronGeometryParams&, const IcosahedronGeometryParams&) = default;

        float radius = 1.0f;
        size_t detail = 0;
    };

    class IcosahedronGeometry final {
    public:
        using Params = IcosahedronGeometryParams;

        static constexpr CStringView name() { return "Icosahedron"; }

        explicit IcosahedronGeometry(const Params& = {});

        const Mesh& mesh() const { return mesh_; }
        operator const Mesh& () const { return mesh_; }
    private:
        Mesh mesh_;
    };
}
