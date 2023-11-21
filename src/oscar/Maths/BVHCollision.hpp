#pragma once

#include <oscar/Maths/RayCollision.hpp>
#include <oscar/Maths/Vec3.hpp>

#include <cstddef>

namespace osc
{
    struct BVHCollision final : public RayCollision {

        BVHCollision(
            float distance_,
            Vec3 const& position_,
            ptrdiff_t id_) :

            RayCollision{distance_, position_},
            id{id_}
        {
        }

        ptrdiff_t id{};
    };
}
