#pragma once

#include "src/Maths/AABB.hpp"

#include <glm/vec3.hpp>
#include <nonstd/span.hpp>

#include <vector>
#include <cstddef>
#include <cstdint>

namespace osc
{
    struct Line;
}

namespace osc
{
    struct BVHNode final {
        AABB bounds;  // union of all AABBs below/including this one
        int nlhs;  // number of nodes in left-hand side, -1 if this node is a leaf
        int firstPrimOffset;  // offset into prim array, -1 if this node is internal
        int nPrims;  // number of prims this node represents
    };

    struct BVHPrim final {
        int id;  // ID into source collection (e.g. a mesh instance, a triangle)
        AABB bounds;  // AABB of the prim in the source collection
    };

    struct BVH final {
        std::vector<BVHNode> nodes;
        std::vector<BVHPrim> prims;

        void clear();
    };

    struct BVHCollision final {
        int primId;
        float distance;
    };

    // triangle BVHes
    //
    // these are BVHes where prim.id refers to the first index of a triangle

    // prim.id will refer to the index of the first vertex in the triangle
    void BVH_BuildFromIndexedTriangles(BVH&, nonstd::span<glm::vec3 const> verts, nonstd::span<uint16_t const> indices);
    void BVH_BuildFromIndexedTriangles(BVH&, nonstd::span<glm::vec3 const> verts, nonstd::span<uint32_t const> indices);

    // populates `out` with the closest collision along the ray - if there is a collision
    //
    // returns `true` if there was a collision; otherwise, `false` and `out` if left untouched
    bool BVH_GetClosestRayIndexedTriangleCollision(BVH const&, nonstd::span<glm::vec3 const> verts, nonstd::span<uint16_t const> indices, Line const&, BVHCollision* out);
    bool BVH_GetClosestRayIndexedTriangleCollision(BVH const&, nonstd::span<glm::vec3 const> verts, nonstd::span<uint32_t const> indices, Line const&, BVHCollision* out);


    // AABB BVHes
    //
    // these are BVHes where prim.id refers to the index of the AABB the node was built from

    // prim.id will refer to the index of the AABB
    void BVH_BuildFromAABBs(BVH&, AABB const*, size_t naabbs);

    // returns prim.id of the AABB (leaf) that the line intersects, or -1 if no intersection
    //
    // no assumptions about prim.id required here - it's using the BVH's AABBs
    //
    // returns true if at least one collision was found and appended to the output
    bool BVH_GetRayAABBCollisions(BVH const&, Line const&, std::vector<BVHCollision>* appendTo);
}
