#pragma once

#include <oscar/Maths/AABB.h>

#include <cstddef>

namespace osc
{
    // a primitive (leaf) in the BVH
    //
    // IDs and bounds an element from the input data (triangles/AABBs)
    class BVHPrim final {
    public:
        BVHPrim(ptrdiff_t id, const AABB& bounds) :
            id_{id},
            bounds_{bounds}
        {}

        ptrdiff_t id() const { return id_; }
        const AABB& bounds() const { return bounds_; }

    private:
        ptrdiff_t id_{};
        AABB bounds_{};
    };
}
