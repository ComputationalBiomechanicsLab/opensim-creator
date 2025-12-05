#pragma once

#include <liboscar/Maths/RayCollision.h>
#include <liboscar/Maths/Vector3.h>

#include <cstddef>

namespace osc
{
    // a collision between a ray and a leaf of a BVH
    struct BVHCollision final : public RayCollision {

        BVHCollision(
            float distance_,
            const Vector3& position_,
            ptrdiff_t id_) :

            RayCollision{distance_, position_},
            id{id_}
        {}

        ptrdiff_t id{};
    };
}
