#include "bvh.hpp"

#include "src/assertions.hpp"
#include "src/3d/model.hpp"

#include <algorithm>
#include <limits>
#include <utility>
#include <cstddef>

using namespace osc;

// recursively build the BVH
static void BVH_RecursiveBuild(BVH& bvh, int begin, int n) {
    if (n == 0) {
        return;
    }

    int end = begin + n;

    // if recursion bottoms out, create leaf node
    if (n == 1) {
        BVH_Node& leaf = bvh.nodes.emplace_back();
        leaf.bounds = bvh.prims[begin].bounds;
        leaf.nlhs = -1;
        leaf.firstPrimOffset = begin;
        leaf.nPrims = 1;
        return;
    }

    // else: compute internal node
    OSC_ASSERT(n > 1 && "trying to treat a lone node as if it were an internal node - this shouldn't be possible (the implementation should have already handled the leaf case)");

    // compute bounding box of remaining prims
    AABB aabb = bvh.prims[begin].bounds;
    for (int i = begin + 1; i < end; ++i) {
        aabb = aabb_union(aabb, bvh.prims[i].bounds);
    }

    // edge-case: if it's empty, return a leaf node
    if (aabb_is_empty(aabb)) {
        BVH_Node& leaf = bvh.nodes.emplace_back();
        leaf.bounds = aabb;
        leaf.nlhs = -1;
        leaf.firstPrimOffset = begin;
        leaf.nPrims = n;
        return;
    }

    // compute slicing position along the longest dimension
    auto dim_idx = aabb_longest_dim_idx(aabb);
    float midpoint_x2 = aabb.min[dim_idx] + aabb.max[dim_idx];

    // returns true if a given primitive is below the midpoint along the dim
    auto is_below_midpoint = [dim_idx, midpoint_x2](BVH_Prim const& p) {
        float prim_midpoint_x2 = p.bounds.min[dim_idx] + p.bounds.max[dim_idx];
        return prim_midpoint_x2 <= midpoint_x2;
    };

    // partition prims into above/below the midpoint
    auto it = std::partition(bvh.prims.begin() + begin, bvh.prims.begin() + end, is_below_midpoint);
    int mid = static_cast<int>(std::distance(bvh.prims.begin(), it));

    // edge-case: failed to spacially partition: just naievely partition
    if (!(begin < mid && mid < end)) {
        mid = begin + n/2;
    }

    OSC_ASSERT(begin < mid && mid < end && "BVH partitioning failed to create two partitions - this shouldn't be possible");

    // allocate internal node
    int internal_loc = static_cast<int>(bvh.nodes.size());  // careful: reallocations
    bvh.nodes.emplace_back();
    bvh.nodes[internal_loc].firstPrimOffset = -1;
    bvh.nodes[internal_loc].nPrims = 0;

    // build left-hand subtree
    BVH_RecursiveBuild(bvh, begin, mid-begin);

    // the left-hand build allocated nodes for the left hand side contiguously in memory
    int lhs_sz = static_cast<int>(bvh.nodes.size() - 1) - internal_loc;
    OSC_ASSERT(lhs_sz > 0);
    bvh.nodes[internal_loc].nlhs = lhs_sz;

    // build right node
    BVH_RecursiveBuild(bvh, mid, end - mid);
    OSC_ASSERT(internal_loc+lhs_sz < static_cast<int>(bvh.nodes.size()));

    // compute internal node's bounds from the left+right side
    AABB const& aabb_lhs = bvh.nodes[internal_loc+1].bounds;
    AABB const& aabb_rhs = bvh.nodes[internal_loc+1+lhs_sz].bounds;
    bvh.nodes[internal_loc].bounds = aabb_union(aabb_lhs, aabb_rhs);
}

// returns true if something hit (the return value is only used in recursion)
//
// populates outparam with all triangle hits in depth-first order
static bool BVH_get_ray_collisions_triangles_recursive(
        BVH const& bvh,
        glm::vec3 const* vs,
        size_t n,
        Line const& ray,
        int nodeidx,
        std::vector<BVH_Collision>& out) {

    BVH_Node const& node = bvh.nodes[nodeidx];

    // check ray-AABB intersection with the BVH node
    Ray_collision res = get_ray_collision_AABB(ray, node.bounds);

    if (!res.hit) {
        return false;  // no intersection with this node at all
    }

    if (node.nlhs == -1) {
        // leaf node: check ray-triangle intersection

        bool hit = false;
        for (int i = node.firstPrimOffset, end = node.firstPrimOffset + node.nPrims; i < end; ++i) {
            BVH_Prim const& p = bvh.prims[i];

            Ray_collision rayrtri = get_ray_collision_triangle(ray, vs + p.id);
            if (rayrtri.hit) {
                out.push_back(BVH_Collision{p.id, rayrtri.distance});
                hit = true;
            }
        }
        return hit;
    } else {
        // else: internal node: check intersection with direct children

        bool lhs = BVH_get_ray_collisions_triangles_recursive(bvh, vs, n, ray, nodeidx+1, out);
        bool rhs = BVH_get_ray_collisions_triangles_recursive(bvh, vs, n, ray, nodeidx+node.nlhs+1, out);
        return lhs || rhs;
    }
}

// returns true if something hit (recursively)
//
// populates outparam with all AABB hits in depth-first order
static bool BVH_get_ray_collision_AABBs_recursive(
        BVH const& bvh,
        Line const& ray,
        int nodeidx,
        std::vector<BVH_Collision>& out) {

    BVH_Node const& node = bvh.nodes[nodeidx];

    // check ray-AABB intersection with the BVH node
    Ray_collision res = get_ray_collision_AABB(ray, node.bounds);

    if (!res.hit) {
        return false;  // no intersection with this node at all
    }

    if (node.nlhs == -1) {
        // it's a leaf node, so we've sucessfully found the AABB that intersected

        out.push_back(BVH_Collision{bvh.prims[node.firstPrimOffset].id, res.distance});
        return true;
    }

    // else: we've "hit" an internal node and need to recurse to find the leaf

    bool lhs = BVH_get_ray_collision_AABBs_recursive(bvh, ray, nodeidx+1, out);
    bool rhs = BVH_get_ray_collision_AABBs_recursive(bvh, ray, nodeidx+node.nlhs+1, out);
    return lhs || rhs;
}

static bool BVH_get_closest_collision_triangle_recursive(
        BVH const& bvh,
        glm::vec3 const* verts,
        size_t nverts,
        Line const& ray,
        float& closest,
        int nodeidx,
        BVH_Collision* out) {

    BVH_Node const& node = bvh.nodes[nodeidx];
    Ray_collision res = get_ray_collision_AABB(ray, node.bounds);

    if (!res.hit) {
        return false;  // didn't hit this node at all
    }

    if (res.distance > closest) {
        return false;  // this AABB can't contain something closer
    }

    if (node.nlhs == -1) {
        // leaf node: check ray-triangle intersection

        bool hit = false;
        for (int i = node.firstPrimOffset, end = node.firstPrimOffset + node.nPrims; i < end; ++i) {
            BVH_Prim const& p = bvh.prims[i];

            Ray_collision rayrtri = get_ray_collision_triangle(ray, verts + p.id);

            if (rayrtri.hit && rayrtri.distance < closest) {
                closest = rayrtri.distance;
                out->prim_id = p.id;
                out->distance = rayrtri.distance;
                hit = true;
            }
        }

        return hit;
    }

    // else: internal node: recurse
    bool lhs = BVH_get_closest_collision_triangle_recursive(bvh, verts, nverts, ray, closest, nodeidx+1, out);
    bool rhs = BVH_get_closest_collision_triangle_recursive(bvh, verts, nverts, ray, closest, nodeidx+node.nlhs+1, out);
    return lhs || rhs;
}

void osc::BVH::clear() {
    nodes.clear();
    prims.clear();
}

BVH osc::BVH_CreateFromTriangles(glm::vec3 const* vs, size_t n) {
    BVH rv;
    BVH_BuildFromTriangles(rv, vs, n);
    return rv;
}

void osc::BVH_BuildFromTriangles(BVH& bvh, glm::vec3 const* vs, size_t n) {
    // clear out any old data
    bvh.clear();

    // build up the prim list for each triangle
    for (size_t i = 0; i < n; i += 3) {
        BVH_Prim& prim = bvh.prims.emplace_back();
        prim.bounds = aabb_from_points(vs + i, 3);
        prim.id = static_cast<int>(i);
    }

    // recursively build the tree
    BVH_RecursiveBuild(bvh, 0, static_cast<int>(bvh.prims.size()));
}

void osc::BVH_BuildFromAABBs(BVH& bvh, AABB const* aabbs, size_t n) {
    // clear out any old data
    bvh.nodes.clear();
    bvh.prims.clear();

    // build up prim list for each AABB (just copy the AABB)
    for (size_t i = 0; i < n; ++i) {
        BVH_Prim& prim = bvh.prims.emplace_back();
        prim.bounds = aabbs[i];
        prim.id = static_cast<int>(i);
    }

    // recursively build the tree
    BVH_RecursiveBuild(bvh, 0, static_cast<int>(bvh.prims.size()));
}

bool osc::BVH_get_ray_collisions_triangles(
        BVH const& bvh,
        glm::vec3 const* vs,
        size_t n,
        Line const& ray,
        std::vector<BVH_Collision>* appendTo) {

    OSC_ASSERT(appendTo != nullptr);
    OSC_ASSERT(n/3 == bvh.prims.size() && "not enough primitives in this BVH - did you build it against the supplied verts?");

    if (bvh.nodes.empty()) {
        return false;
    }

    if (bvh.prims.empty()) {
        return false;
    }

    if (n == 0) {
        return false;
    }

    return BVH_get_ray_collisions_triangles_recursive(bvh, vs, n, ray, 0, *appendTo);
}

bool osc::BVH_get_ray_collision_AABBs(
        BVH const& bvh,
        Line const& ray,
        std::vector<BVH_Collision>* appendTo) {

    OSC_ASSERT(appendTo != nullptr);

    if (bvh.nodes.empty()) {
        return false;
    }

    if (bvh.prims.empty()) {
        return false;
    }

    return BVH_get_ray_collision_AABBs_recursive(bvh, ray, 0, *appendTo);
}

bool osc::BVH_get_closest_collision_triangle(
        BVH const& bvh,
        glm::vec3 const* verts,
        size_t nverts,
        Line const& ray,
        BVH_Collision* out) {

    OSC_ASSERT(out != nullptr);
    OSC_ASSERT(nverts/3 == bvh.prims.size() && "not enough primitives in this BVH - did you build it against the supplied verts?");

    if (bvh.nodes.empty()) {
        return false;
    }

    if (bvh.prims.empty()) {
        return false;
    }

    if (nverts == 0) {
        return false;
    }

    float closest = std::numeric_limits<float>::max();
    return BVH_get_closest_collision_triangle_recursive(bvh, verts, nverts, ray, closest, 0, out);
}

