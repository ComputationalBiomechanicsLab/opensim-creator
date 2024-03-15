#pragma once

#include <oscar/Maths/AABB.h>
#include <oscar/Maths/BVHCollision.h>
#include <oscar/Maths/BVHNode.h>
#include <oscar/Maths/BVHPrim.h>
#include <oscar/Maths/Vec3.h>

#include <cstdint>
#include <cstddef>
#include <functional>
#include <optional>
#include <span>
#include <vector>

namespace osc { struct Line; }

namespace osc
{
    // a bounding volume hierarchy (BVH) of numerically IDed AABBs
    //
    // the AABBs may be computed from triangles, commonly called a "triangle BVH"
    class BVH final {
    public:
        void clear();

        // triangle BVHes
        //
        // prim.getID() will refer to the index of the first vertex in the triangle
        void buildFromIndexedTriangles(
            std::span<Vec3 const> verts,
            std::span<uint16_t const> indices
        );
        void buildFromIndexedTriangles(
            std::span<Vec3 const> verts,
            std::span<uint32_t const> indices
        );

        // returns the location of the closest ray-triangle collision along the ray, if any
        std::optional<BVHCollision> getClosestRayIndexedTriangleCollision(
            std::span<Vec3 const> verts,
            std::span<uint16_t const> indices,
            Line const&
        ) const;
        std::optional<BVHCollision> getClosestRayIndexedTriangleCollision(
            std::span<Vec3 const> verts,
            std::span<uint32_t const> indices,
            Line const&
        ) const;

        // AABB BVHes
        //
        // prim.id will refer to the index of the AABB
        void buildFromAABBs(std::span<AABB const>);

        // calls the callback with each collision between the line and an AABB in
        // the BVH
        void forEachRayAABBCollision(Line const&, std::function<void(BVHCollision)> const&) const;

        // returns `true` if the BVH contains no nodes
        [[nodiscard]] bool empty() const;

        // returns the maximum depth of the given BVH tree
        size_t getMaxDepth() const;

        // returns the AABB of the root node, or `std::nullopt` if there are no nodes in
        // the tree
        std::optional<AABB> getBounds() const;

        // calls the given function with each leaf node in the tree
        void forEachLeafNode(std::function<void(BVHNode const&)> const&) const;

        // calls the given function with each leaf or inner node in the tree
        void forEachLeafOrInnerNodeUnordered(std::function<void(BVHNode const&)> const&) const;

    private:
        std::vector<BVHNode> m_Nodes;  // nodes in the hierarchy
        std::vector<BVHPrim> m_Prims;  // primitives (triangles, AABBs) that the nodes reference
    };
}
