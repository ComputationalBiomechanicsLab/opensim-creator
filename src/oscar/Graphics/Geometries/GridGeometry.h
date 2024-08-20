#pragma once

#include <oscar/Graphics/Mesh.h>
#include <oscar/Utils/CStringView.h>

#include <cstddef>

namespace osc
{
    struct GridGeometryParams final {
        friend bool operator==(const GridGeometryParams&, const GridGeometryParams&) = default;

        float size = 2.0f;
        size_t num_divisions = 10;
    };

    class GridGeometry final : public Mesh {
    public:
        using Params = GridGeometryParams;

        static constexpr CStringView name() { return "Grid"; }

        explicit GridGeometry(const Params& = {});
    };
}
