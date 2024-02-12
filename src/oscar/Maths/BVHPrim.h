#pragma once

#include <oscar/Maths/AABB.h>

#include <cstddef>

namespace osc
{
    class BVHPrim final {
    public:
        BVHPrim(ptrdiff_t id_, AABB const& bounds_) :
            m_ID{id_},
            m_Bounds{bounds_}
        {
        }

        ptrdiff_t getID() const
        {
            return m_ID;
        }

        AABB const& getBounds() const
        {
            return m_Bounds;
        }

    private:
        ptrdiff_t m_ID{};
        AABB m_Bounds{};
    };
}
