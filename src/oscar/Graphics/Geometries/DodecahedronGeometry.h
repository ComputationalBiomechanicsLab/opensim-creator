#pragma once

#include <oscar/Graphics/Mesh.h>

#include <cstddef>

namespace osc
{
    class DodecahedronGeometry final {
    public:
        static Mesh generate_mesh(
            float radius = 1.0f,
            size_t detail = 0
        );
    };
}
