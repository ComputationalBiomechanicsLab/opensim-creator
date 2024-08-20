#pragma once

#include <oscar/Graphics/Mesh.h>
#include <oscar/Utils/CStringView.h>

#include <cstddef>

namespace osc
{
    struct DodecahedronGeometryParams final {
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
