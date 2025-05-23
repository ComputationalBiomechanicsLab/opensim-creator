#pragma once

#include <liboscar/Graphics/Mesh.h>
#include <liboscar/Utils/CStringView.h>

#include <cstddef>

namespace osc
{
    struct DodecahedronGeometryParams final {
        friend bool operator==(const DodecahedronGeometryParams&, const DodecahedronGeometryParams&) = default;

        float radius = 1.0f;
        size_t detail = 0;
    };

    class DodecahedronGeometry final : public Mesh {
    public:
        using Params = DodecahedronGeometryParams;

        static constexpr CStringView name() { return "Dodecahedron"; }

        explicit DodecahedronGeometry(const Params& = {});
    };
}
