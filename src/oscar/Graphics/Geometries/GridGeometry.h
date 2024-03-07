#pragma once

#include <oscar/Graphics/Mesh.h>

#include <cstddef>

namespace osc
{
    class GridGeometry final {
    public:
        static Mesh generate_mesh(
            float size = 2.0f,
            size_t divisions = 10
        );
    };
}
