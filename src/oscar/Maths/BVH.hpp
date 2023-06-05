#pragma once

#include "oscar/Maths/AABB.hpp"
#include "oscar/Maths/RayCollision.hpp"

#include <glm/vec3.hpp>
#include <nonstd/span.hpp>

#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

namespace osc { struct Line; }

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
            m_Data{std::move(data_)}
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
        static inline size_t constexpr c_LeafMask = static_cast<size_t>(1) << (8*sizeof(size_t) - 1);
        AABB m_Bounds;  // union of all AABBs below/including this one
        size_t m_Data;
    };

    class BVHPrim final {
    public:
        BVHPrim(ptrdiff_t id_, AABB const& bounds_) :
            m_ID{std::move(id_)},
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
        ptrdiff_t m_ID;
        AABB m_Bounds;
    };

    struct BVHCollision final : public RayCollision {

        BVHCollision(float distance_, glm::vec3 position_, ptrdiff_t id_) :
            RayCollision{distance_, position_},
            id{id_}
        {
        }

        ptrdiff_t id;
    };

    // TODO: should be a class
    struct BVH final {

        void clear();

        // triangle BVHes
        //
        // prim.getID() will refer to the index of the first vertex in the triangle
        void buildFromIndexedTriangles(
            nonstd::span<glm::vec3 const> verts,
            nonstd::span<uint16_t const> indices
        );
        void buildFromIndexedTriangles(
            nonstd::span<glm::vec3 const> verts,
            nonstd::span<uint32_t const> indices
        );

        // returns the location of the closest ray-triangle collision along the ray, if any
        std::optional<BVHCollision> getClosestRayIndexedTriangleCollision(
            nonstd::span<glm::vec3 const> verts,
            nonstd::span<uint16_t const> indices,
            Line const&
        ) const;
        std::optional<BVHCollision> getClosestRayIndexedTriangleCollision(
            nonstd::span<glm::vec3 const> verts,
            nonstd::span<uint32_t const> indices,
            Line const&
        ) const;

        // AABB BVHes
        //
        // prim.id will refer to the index of the AABB
        void buildFromAABBs(nonstd::span<AABB const> aabbs);

        // returns prim.id of the AABB (leaf) that the line intersects, or -1 if no intersection
        //
        // no assumptions about prim.id required here - it's using the BVH's AABBs
        //
        // returns true if at least one collision was found and appended to the output
        std::vector<BVHCollision> getRayAABBCollisions(Line const&) const;

        // returns the maximum depth of the given BVH tree
        size_t getMaxDepth() const;

        // returns the AABB of the root node, or `std::nullopt` if there are no nodes in
        // the tree
        std::optional<AABB> getRootAABB() const;

        std::vector<BVHNode> nodes;
        std::vector<BVHPrim> prims;
    };
}
