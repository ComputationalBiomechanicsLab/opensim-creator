#pragma once

#include <oscar/Graphics/Mesh.h>
#include <oscar/Maths/AABB.h>

namespace osc
{
    class AABBGeometry final {
    public:
        static Mesh generate_mesh(
            AABB const& = {.min = {-1.0f, -1.0f, -1.0f}, .max = {1.0f, 1.0f, 1.0f}}
        );
    };
}
