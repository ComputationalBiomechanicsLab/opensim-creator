#pragma once

#include <oscar/Graphics/Mesh.h>
#include <oscar/Utils/CStringView.h>

#include <cstddef>

namespace osc
{
    class GridGeometry final : public Mesh {
    public:
        static constexpr CStringView name() { return "Grid"; }

        struct Params final {
            float size = 2.0f;
            size_t num_divisions = 10;
        };

        explicit GridGeometry(const Params& = {});
    };
}
