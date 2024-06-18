#pragma once

#include <oscar/Graphics/Mesh.h>
#include <oscar/Maths/AABB.h>
#include <oscar/Utils/CStringView.h>

namespace osc
{
    class AABBGeometry final {
    public:
        static constexpr CStringView name() { return "AABB"; }

        AABBGeometry(const AABB& = {.min = {-1.0f, -1.0f, -1.0f}, .max = {1.0f, 1.0f, 1.0f}});

        const Mesh& mesh() const { return mesh_; }
        operator const Mesh& () const { return mesh_; }
    private:
        Mesh mesh_;
    };
}
