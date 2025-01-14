#pragma once

#include <liboscar/Graphics/Mesh.h>
#include <liboscar/Utils/CStringView.h>

#include <cstddef>

namespace osc
{
    struct IcosahedronGeometryParams final {
        friend bool operator==(const IcosahedronGeometryParams&, const IcosahedronGeometryParams&) = default;

        float radius = 1.0f;
        size_t detail = 0;
    };

    class IcosahedronGeometry final : public Mesh {
    public:
        using Params = IcosahedronGeometryParams;

        static constexpr CStringView name() { return "Icosahedron"; }

        explicit IcosahedronGeometry(const Params& = {});
    };
}
