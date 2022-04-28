#include "BVH.hpp"

#include "src/Maths/AABB.hpp"
#include "src/Maths/Geometry.hpp"
#include "src/Maths/Line.hpp"
#include "src/Maths/RayCollision.hpp"
#include "src/Utils/Assertions.hpp"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <utility>
#include <cstddef>

// recursively build the BVH
static void BVH_RecursiveBuild(osc::BVH& bvh, int begin, int n)
{
    if (n == 0)
    {
        return;
    }

    int end = begin + n;

    // if recursion bottoms out, create leaf node
    if (n == 1)
    {
        osc::BVHNode& leaf = bvh.nodes.emplace_back();
        leaf.bounds = bvh.prims[begin].bounds;
        leaf.nlhs = -1;
        leaf.firstPrimOffset = begin;
        leaf.nPrims = 1;
        return;
    }

    // else: compute internal node
    OSC_ASSERT(n > 1 && "trying to treat a lone node as if it were an internal node - this shouldn't be possible (the implementation should have already handled the leaf case)");

    // compute bounding box of remaining prims
    osc::AABB aabb = Union(bvh.prims.data() + begin,
                           end-begin,
                           sizeof(osc::BVHPrim),
                           offsetof(osc::BVHPrim, bounds));

    // edge-case: if it's empty, return a leaf node
    if (IsEffectivelyEmpty(aabb))
    {
        osc::BVHNode& leaf = bvh.nodes.emplace_back();
        leaf.bounds = aabb;
        leaf.nlhs = -1;
        leaf.firstPrimOffset = begin;
        leaf.nPrims = n;
        return;
    }

    // compute slicing position along the longest dimension
    auto longestDimIdx = LongestDimIndex(aabb);
    float midpointX2 = aabb.min[longestDimIdx] + aabb.max[longestDimIdx];

    // returns true if a given primitive is below the midpoint along the dim
    auto isBelowMidpoint = [longestDimIdx, midpointX2](osc::BVHPrim const& p)
    {
        float primMidpointX2 = p.bounds.min[longestDimIdx] + p.bounds.max[longestDimIdx];
        return primMidpointX2 <= midpointX2;
    };

    // partition prims into above/below the midpoint
    auto it = std::partition(bvh.prims.begin() + begin, bvh.prims.begin() + end, isBelowMidpoint);
    int mid = static_cast<int>(std::distance(bvh.prims.begin(), it));

    // edge-case: failed to spacially partition: just naievely partition
    if (!(begin < mid && mid < end))
    {
        mid = begin + n/2;
    }

    OSC_ASSERT(begin < mid && mid < end && "BVH partitioning failed to create two partitions - this shouldn't be possible");

    // allocate internal node
    int internalNodeLoc = static_cast<int>(bvh.nodes.size());  // careful: reallocations
    bvh.nodes.emplace_back();
    bvh.nodes[internalNodeLoc].firstPrimOffset = -1;
    bvh.nodes[internalNodeLoc].nPrims = 0;

    // build left-hand subtree
    BVH_RecursiveBuild(bvh, begin, mid-begin);

    // the left-hand build allocated nodes for the left hand side contiguously in memory
    int numLhsNodes = static_cast<int>(bvh.nodes.size() - 1) - internalNodeLoc;
    OSC_ASSERT(numLhsNodes > 0);
    bvh.nodes[internalNodeLoc].nlhs = numLhsNodes;

    // build right node
    BVH_RecursiveBuild(bvh, mid, end - mid);
    OSC_ASSERT(internalNodeLoc+numLhsNodes < static_cast<int>(bvh.nodes.size()));

    // compute internal node's bounds from the left+right side
    osc::AABB const& lhsAABB = bvh.nodes[internalNodeLoc+1].bounds;
    osc::AABB const& rhsAABB = bvh.nodes[internalNodeLoc+1+numLhsNodes].bounds;
    bvh.nodes[internalNodeLoc].bounds = Union(lhsAABB, rhsAABB);
}

// returns true if something hit (the return value is only used in recursion)
//
// populates outparam with all triangle hits in depth-first order
static bool BVH_GetRayTriangleCollisionsRecursive(
        osc::BVH const& bvh,
        glm::vec3 const* vs,
        size_t n,
        osc::Line const& ray,
        int nodeidx,
        std::vector<osc::BVHCollision>& out)
{
    osc::BVHNode const& node = bvh.nodes[nodeidx];

    // check ray-AABB intersection with the BVH node
    osc::RayCollision res = osc::GetRayCollisionAABB(ray, node.bounds);

    if (!res.hit)
    {
        return false;  // no intersection with this node at all
    }

    if (node.nlhs == -1)
    {
        // leaf node: check ray-triangle intersection

        bool hit = false;
        for (int i = node.firstPrimOffset, end = node.firstPrimOffset + node.nPrims; i < end; ++i)
        {
            osc::BVHPrim const& p = bvh.prims[i];

            osc::RayCollision rayrtri = osc::GetRayCollisionTriangle(ray, vs + p.id);
            if (rayrtri.hit)
            {
                out.push_back(osc::BVHCollision{p.id, rayrtri.distance});
                hit = true;
            }
        }
        return hit;
    }
    else
    {
        // else: internal node: check intersection with direct children

        bool lhs = BVH_GetRayTriangleCollisionsRecursive(bvh, vs, n, ray, nodeidx+1, out);
        bool rhs = BVH_GetRayTriangleCollisionsRecursive(bvh, vs, n, ray, nodeidx+node.nlhs+1, out);
        return lhs || rhs;
    }
}

// make this public if we ever need multi-collisions
static bool BVH_GetRayTriangleCollisions(
    osc::BVH const& bvh,
    glm::vec3 const* vs,
    size_t n,
    osc::Line const& ray,
    std::vector<osc::BVHCollision>* appendTo)
{
    OSC_ASSERT(appendTo != nullptr);
    OSC_ASSERT(n/3 == bvh.prims.size() && "not enough primitives in this BVH - did you build it against the supplied verts?");

    if (bvh.nodes.empty())
    {
        return false;
    }

    if (bvh.prims.empty())
    {
        return false;
    }

    if (n == 0)
    {
        return false;
    }

    return BVH_GetRayTriangleCollisionsRecursive(bvh, vs, n, ray, 0, *appendTo);
}

// returns true if something hit (recursively)
//
// populates outparam with all AABB hits in depth-first order
static bool BVH_GetRayAABBCollisionsRecursive(
        osc::BVH const& bvh,
        osc::Line const& ray,
        int nodeidx,
        std::vector<osc::BVHCollision>& out)
{
    osc::BVHNode const& node = bvh.nodes[nodeidx];

    // check ray-AABB intersection with the BVH node
    osc::RayCollision res = osc::GetRayCollisionAABB(ray, node.bounds);

    if (!res.hit)
    {
        return false;  // no intersection with this node at all
    }

    if (node.nlhs == -1)
    {
        // it's a leaf node, so we've sucessfully found the AABB that intersected

        out.push_back(osc::BVHCollision{bvh.prims[node.firstPrimOffset].id, res.distance});
        return true;
    }

    // else: we've "hit" an internal node and need to recurse to find the leaf

    bool lhs = BVH_GetRayAABBCollisionsRecursive(bvh, ray, nodeidx+1, out);
    bool rhs = BVH_GetRayAABBCollisionsRecursive(bvh, ray, nodeidx+node.nlhs+1, out);
    return lhs || rhs;
}

template<typename TIndex>
static bool BVH_GetClosestRayIndexedTriangleCollisionRecursive(
        osc::BVH const& bvh,
        nonstd::span<glm::vec3 const> verts,
        nonstd::span<TIndex const> indices,
        osc::Line const& ray,
        float& closest,
        int nodeidx,
        osc::BVHCollision* out)
{
    osc::BVHNode const& node = bvh.nodes[nodeidx];
    osc::RayCollision res = osc::GetRayCollisionAABB(ray, node.bounds);

    if (!res.hit)
    {
        return false;  // didn't hit this node at all
    }

    if (res.distance > closest)
    {
        return false;  // this AABB can't contain something closer
    }

    if (node.nlhs == -1)
    {
        // leaf node: check ray-triangle intersection

        bool hit = false;
        for (int i = node.firstPrimOffset, end = node.firstPrimOffset + node.nPrims; i < end; ++i)
        {
            osc::BVHPrim const& p = bvh.prims[i];

            glm::vec3 triangleVerts[] =
            {
                verts[indices[p.id]],
                verts[indices[p.id + 1]],
                verts[indices[p.id + 2]],
            };

            osc::RayCollision rayrtri = osc::GetRayCollisionTriangle(ray, triangleVerts);

            if (rayrtri.hit && rayrtri.distance < closest)
            {
                closest = rayrtri.distance;
                out->primId = p.id;
                out->distance = rayrtri.distance;
                hit = true;
            }
        }

        return hit;
    }

    // else: internal node: recurse
    bool lhs = BVH_GetClosestRayIndexedTriangleCollisionRecursive(bvh, verts, indices, ray, closest, nodeidx+1, out);
    bool rhs = BVH_GetClosestRayIndexedTriangleCollisionRecursive(bvh, verts, indices, ray, closest, nodeidx+node.nlhs+1, out);
    return lhs || rhs;
}

template<typename TIndex>
static void BuildFromIndexedTriangles(osc::BVH& bvh, nonstd::span<glm::vec3 const> verts, nonstd::span<TIndex const> indices)
{
    // clear out any old data
    bvh.clear();

    // build up the prim list for each triangle
    OSC_ASSERT_ALWAYS(indices.size() % 3 == 0);
    for (size_t i = 0; i < indices.size(); i += 3)
    {
        glm::vec3 triangleVerts[3] =
        {
            verts[indices[i]],
            verts[indices[i+1]],
            verts[indices[i+2]],
        };
        osc::BVHPrim& prim = bvh.prims.emplace_back();
        prim.bounds = osc::AABBFromVerts(triangleVerts, 3);
        prim.id = static_cast<int>(i);
    }

    // recursively build the tree
    BVH_RecursiveBuild(bvh, 0, static_cast<int>(bvh.prims.size()));
}

template<typename TIndex>
static bool GetClosestRayIndexedTriangleCollision(
    osc::BVH const& bvh,
    nonstd::span<glm::vec3 const> verts,
    nonstd::span<TIndex const> indices,
    osc::Line const& ray,
    osc::BVHCollision* out)
{
    OSC_ASSERT(out != nullptr);
    OSC_ASSERT(indices.size()/3 == bvh.prims.size() && "not enough primitives in this BVH - did you build it against the supplied verts?");

    if (bvh.nodes.empty() || bvh.prims.empty() || indices.empty())
    {
        return false;
    }

    float closest = std::numeric_limits<float>::max();
    return BVH_GetClosestRayIndexedTriangleCollisionRecursive(bvh, verts, indices, ray, closest, 0, out);
}


// public API

void osc::BVH::clear()
{
    nodes.clear();
    prims.clear();
}

void osc::BVH_BuildFromIndexedTriangles(BVH& bvh, nonstd::span<glm::vec3 const> verts, nonstd::span<uint16_t const> indices)
{
    BuildFromIndexedTriangles<uint16_t>(bvh, verts, indices);
}

void osc::BVH_BuildFromIndexedTriangles(BVH& bvh, nonstd::span<glm::vec3 const> verts, nonstd::span<uint32_t const> indices)
{
    BuildFromIndexedTriangles<uint32_t>(bvh, verts, indices);
}

bool osc::BVH_GetClosestRayIndexedTriangleCollision(BVH const& bvh, nonstd::span<glm::vec3 const> verts, nonstd::span<uint16_t const> indices, Line const& line, BVHCollision* out)
{
    return GetClosestRayIndexedTriangleCollision<uint16_t>(bvh, verts, indices, line, out);
}

bool osc::BVH_GetClosestRayIndexedTriangleCollision(BVH const& bvh, nonstd::span<glm::vec3 const> verts, nonstd::span<uint32_t const> indices, Line const& line, BVHCollision* out)
{
    return GetClosestRayIndexedTriangleCollision<uint32_t>(bvh, verts, indices, line, out);
}

void osc::BVH_BuildFromAABBs(BVH& bvh, AABB const* aabbs, size_t n)
{
    // clear out any old data
    bvh.nodes.clear();
    bvh.prims.clear();

    // build up prim list for each AABB (just copy the AABB)
    for (size_t i = 0; i < n; ++i)
    {
        BVHPrim& prim = bvh.prims.emplace_back();
        prim.bounds = aabbs[i];
        prim.id = static_cast<int>(i);
    }

    // recursively build the tree
    BVH_RecursiveBuild(bvh, 0, static_cast<int>(bvh.prims.size()));
}

bool osc::BVH_GetRayAABBCollisions(
        BVH const& bvh,
        Line const& ray,
        std::vector<BVHCollision>* appendTo)
{

    OSC_ASSERT(appendTo != nullptr);

    if (bvh.nodes.empty())
    {
        return false;
    }

    if (bvh.prims.empty())
    {
        return false;
    }

    return BVH_GetRayAABBCollisionsRecursive(bvh, ray, 0, *appendTo);
}
