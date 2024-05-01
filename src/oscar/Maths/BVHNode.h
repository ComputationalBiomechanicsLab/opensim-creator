#pragma once

#include <oscar/Maths/AABB.h>

#include <cstddef>

namespace osc
{
    // an inner/leaf node of a BVH
    //
    // has a spatial and hierarchical bounds, plus an index to a `BVHPrim`
    class BVHNode final {
    public:
        static BVHNode leaf(const AABB& bounds, size_t first_prim_offset)
        {
            return BVHNode{bounds, first_prim_offset | c_leaf_mask};
        }

        static BVHNode node(const AABB& bounds, size_t num_lhs_children)
        {
            return BVHNode{bounds, num_lhs_children & ~c_leaf_mask};
        }

    private:
        BVHNode(const AABB& bounds, size_t data) :
            bounds_{bounds},
            data_{data}
        {}
    public:
        const AABB& bounds() const
        {
            return bounds_;
        }

        bool is_leaf() const
        {
            return (data_ & c_leaf_mask) > 0;
        }

        bool is_node() const
        {
            return !is_leaf();
        }

        size_t num_lhs_nodes() const
        {
            return data_ & ~c_leaf_mask;
        }

        void set_num_lhs_nodes(size_t n)
        {
            data_ = n & ~c_leaf_mask;
        }

        size_t first_prim_offset() const
        {
            return data_ & ~c_leaf_mask;
        }

    private:
        static inline constexpr size_t c_leaf_mask = static_cast<size_t>(1) << (8*sizeof(size_t) - 1);

        // the union of all AABBs below, and including, this node
        AABB bounds_{};

        // bit-packed node data
        size_t data_{};
    };
}
