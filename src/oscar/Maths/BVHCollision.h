#pragma once

#include <oscar/Maths/RayCollision.h>
#include <oscar/Maths/Vec3.h>

#include <cstddef>

namespace osc
{
    // a collision between a ray and a leaf of a BVH
    struct BVHCollision final : public RayCollision {

        BVHCollision(
            float distance_,
            const Vec3& position_,
            ptrdiff_t id_) :

            RayCollision{distance_, position_},
            id{id_}
        {}

        ptrdiff_t id{};
    };
}
