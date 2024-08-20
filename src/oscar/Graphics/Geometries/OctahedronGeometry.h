#pragma once

#include <oscar/Graphics/Mesh.h>
#include <oscar/Utils/CStringView.h>

#include <cstddef>

namespace osc
{
    struct OctahedronGeometryParams final {
        friend bool operator==(const OctahedronGeometryParams&, const OctahedronGeometryParams&) = default;

        float radius = 1.0f;
        size_t detail = 0;
    };

    class OctahedronGeometry final : public Mesh {
    public:
        using Params = OctahedronGeometryParams;

        static constexpr CStringView name() { return "Octahedron"; }

        explicit OctahedronGeometry(const Params& = {});
    };
}
