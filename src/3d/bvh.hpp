#pragma once

#include "src/3d/model.hpp"

#include <glm/vec3.hpp>

#include <vector>
#include <cstddef>

namespace osc {
    struct BVH_Node final {
        AABB bounds;  // union of all AABBs below/including this one
        int nlhs;  // number of nodes in left-hand side, -1 if this node is a leaf
        int firstPrimOffset;  // offset into prim array, -1 if this node is internal
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

    // triangle BVHes
    //
    // these are BVHes where prim.id refers to the first index of a triangle

    // convenience form of BVH_BuildFromTriangles
    BVH BVH_CreateFromTriangles(glm::vec3 const*, size_t nverts);

    // prim.id will refer to the index of the first vertex in the triangle
    void BVH_BuildFromTriangles(BVH&, glm::vec3 const*, size_t nverts);

    // appends all collisions along the ray into the outparam
    //
    // assumes prim.id in the BVH is an offset into the (supplied) triangle verts
    //
    // returns true if at least one collision was found and appended to the output
    bool BVH_get_ray_collisions_triangles(BVH const&, glm::vec3 const*, size_t n, Line const&, std::vector<BVH_Collision>* appendTo);

    // populates `out` with the closest collision along the ray - if there is a collision
    //
    // returns `true` if there was a collision; otherwise, `false` and `out` if left untouched
    bool BVH_get_closest_collision_triangle(BVH const&, glm::vec3 const*, size_t nverts, Line const&, BVH_Collision* out);

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
    bool BVH_get_ray_collision_AABBs(BVH const&, Line const&, std::vector<BVH_Collision>* appendTo);
}
