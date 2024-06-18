#pragma once

#include <oscar/Graphics/Mesh.h>
#include <oscar/Utils/CStringView.h>

#include <cstddef>

namespace osc
{
    class GridGeometry final {
    public:
        static constexpr CStringView name() { return "Grid"; }

        GridGeometry(
            float size = 2.0f,
            size_t num_divisions = 10
        );

        const Mesh& mesh() const { return mesh_; }
        operator const Mesh& () const { return mesh_; }
    private:
        Mesh mesh_;
    };
}
