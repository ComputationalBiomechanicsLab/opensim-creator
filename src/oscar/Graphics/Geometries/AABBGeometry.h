#pragma once

#include <oscar/Graphics/Mesh.h>
#include <oscar/Maths/AABB.h>
#include <oscar/Utils/CStringView.h>

namespace osc
{
    class AABBGeometry final : public Mesh {
    public:
        static constexpr CStringView name() { return "AABB"; }

        explicit AABBGeometry(const AABB& = {.min = {-1.0f, -1.0f, -1.0f}, .max = {1.0f, 1.0f, 1.0f}});
    };
}
