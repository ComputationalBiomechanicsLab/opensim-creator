#pragma once

#include "src/3d/model.hpp"

#include <glm/vec3.hpp>

#include <vector>
#include <cstddef>

namespace osc {
    struct BVH_Node final {
        AABB bounds;  // union of all AABBs below/including this one
        int lhs;  // index of left-hand node, -1 if leaf
        int rhs;  // index of right-hand node, -1 if leaf
        int firstPrimOffset;  // offset into prim array
        int nPrims;  // number of prims this node represents
    };

    struct BVH_Prim final {
        int id;  // ID into source collection (e.g. a mesh instance, a triangle)
        AABB bounds;  // AABB of the prim in the source collection
    };

    struct BVH final {
        std::vector<BVH_Node> nodes;
        std::vector<BVH_Prim> prims;

        void clear();
    };

    struct BVH_Collision final {
        int prim_id;
        float distance;
    };

    // effectively:
    //
    //     BVH bvh;
    //     BVH_BuildFromTriangles(vs, n);
    //     return bvh;
    BVH BVH_CreateFromTriangles(glm::vec3 const*, size_t nverts);

    // prim.id will refer to the index of the first vertex in the triangle
    void BVH_BuildFromTriangles(BVH&, glm::vec3 const*, size_t nverts);

    // prim.id will refer to the index of the AABB
    void BVH_BuildFromAABBs(BVH&, AABB const*, size_t naabbs);

    // appends all collisions along the ray into the outparam
    //
    // assumes prim.id in the BVH is an offset into the (supplied) triangle verts
    //
    // returns true if at least one collision was found and appended to the output
    bool BVH_get_ray_collisions_triangles(BVH const&, glm::vec3 const*, size_t n, Line const&, std::vector<BVH_Collision>* appendTo);

    // returns prim.id of the AABB (leaf) that the line intersects, or -1 if no intersection
    //
    // no assumptions about prim.id required here - it's using the BVH's AABBs
    //
    // returns true if at least one collision was found and appended to the output
    bool BVH_get_ray_collision_AABBs(BVH const&, Line const&, std::vector<BVH_Collision>* appendTo);
}
