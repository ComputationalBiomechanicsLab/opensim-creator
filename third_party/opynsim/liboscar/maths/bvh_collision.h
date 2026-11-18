#pragma once

#include <liboscar/maths/ray_collision.h>
#include <liboscar/maths/vector.h>

#include <cstddef>

namespace osc
{
    // a collision between a ray and a leaf of a BVH
    struct BVHCollision final : public RayCollision {

        BVHCollision(float distance, const Vector3& position, ptrdiff_t id) :

            RayCollision{distance, position},
            id{id}
        {}

        ptrdiff_t id{};
    };
}
