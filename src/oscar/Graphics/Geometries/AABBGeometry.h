#pragma once

#include <oscar/Graphics/Mesh.h>
#include <oscar/Maths/AABB.h>

namespace osc
{
    class AABBGeometry final {
    public:
        AABBGeometry(const AABB& = {.min = {-1.0f, -1.0f, -1.0f}, .max = {1.0f, 1.0f, 1.0f}});

        const Mesh& mesh() const { return mesh_; }
        operator const Mesh& () const { return mesh_; }
    private:
        Mesh mesh_;
    };
}
