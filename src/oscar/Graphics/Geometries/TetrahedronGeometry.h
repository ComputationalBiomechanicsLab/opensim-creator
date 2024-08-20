#pragma once

#include <oscar/Graphics/Mesh.h>
#include <oscar/Utils/CStringView.h>

#include <cstddef>

namespace osc
{
    struct TetrahedronGeometryParams final {
        float radius = 1.0f;
        size_t detail_level = 0;
    };

    class TetrahedronGeometry final : public Mesh {
    public:
        using Params = TetrahedronGeometryParams;

        static constexpr CStringView name() { return "Tetrahedron"; }

        explicit TetrahedronGeometry(const Params& = {});
    };
}
