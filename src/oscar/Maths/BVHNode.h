#pragma once

#include <oscar/Maths/AABB.h>

#include <cstddef>

namespace osc
{
    class BVHNode final {
    public:
        static BVHNode leaf(AABB const& bounds, size_t primOffset)
        {
            return BVHNode{bounds, primOffset | c_LeafMask};
        }

        static BVHNode node(AABB const& bounds, size_t numLhs)
        {
            return BVHNode{bounds, numLhs & ~c_LeafMask};
        }

    private:
        BVHNode(AABB const& bounds_, size_t data_) :
            m_Bounds{bounds_},
            m_Data{data_}
        {
        }
    public:
        AABB const& getBounds() const
        {
            return m_Bounds;
        }

        bool isLeaf() const
        {
            return (m_Data & c_LeafMask) > 0;
        }

        bool isNode() const
        {
            return !isLeaf();
        }

        size_t getNumLhsNodes() const
        {
            return m_Data & ~c_LeafMask;
        }

        void setNumLhsNodes(size_t n)
        {
            m_Data = n & ~c_LeafMask;
        }

        size_t getFirstPrimOffset() const
        {
            return m_Data & ~c_LeafMask;
        }

    private:
        static inline constexpr size_t c_LeafMask = static_cast<size_t>(1) << (8*sizeof(size_t) - 1);
        AABB m_Bounds{};  // union of all AABBs below/including this one
        size_t m_Data{};
    };
}
