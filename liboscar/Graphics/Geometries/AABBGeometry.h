#pragma once

#include <liboscar/Graphics/Mesh.h>
#include <liboscar/Maths/AABB.h>
#include <liboscar/Utils/CStringView.h>

namespace osc
{
    class AABBGeometry final {
    public:
        static constexpr CStringView name() { return "AABB"; }

        explicit AABBGeometry(const AABB& = {.min = {-1.0f, -1.0f, -1.0f}, .max = {1.0f, 1.0f, 1.0f}});

        const Mesh& mesh() const { return mesh_; }
        operator const Mesh& () const { return mesh_; }
    private:
        Mesh mesh_;
    };
}
