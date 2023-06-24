#include "oscar/Bindings/GlmHelpers.hpp"
#include "oscar/Maths/AABB.hpp"
#include "oscar/Maths/BVH.hpp"
#include "oscar/Maths/CollisionTests.hpp"
#include "oscar/Maths/Constants.hpp"
#include "oscar/Maths/Disc.hpp"
#include "oscar/Maths/MathHelpers.hpp"
#include "oscar/Maths/RayCollision.hpp"
#include "oscar/Maths/EulerPerspectiveCamera.hpp"
#include "oscar/Maths/Line.hpp"
#include "oscar/Maths/MathHelpers.hpp"
#include "oscar/Maths/Plane.hpp"
#include "oscar/Maths/PolarPerspectiveCamera.hpp"
#include "oscar/Maths/RayCollision.hpp"
#include "oscar/Maths/Rect.hpp"
#include "oscar/Maths/Segment.hpp"
#include "oscar/Maths/Sphere.hpp"
#include "oscar/Maths/Tetrahedron.hpp"
#include "oscar/Maths/Transform.hpp"
#include "oscar/Maths/Triangle.hpp"
#include "oscar/Utils/Assertions.hpp"

#include <glm/glm.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/quaternion.hpp>
#include <nonstd/span.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <iterator>
#include <iostream>
#include <limits>
#include <memory>
#include <numeric>
#include <stack>
#include <stdexcept>
#include <utility>


// osc::AABB implementation

std::ostream& osc::operator<<(std::ostream& o, AABB const& aabb)
{
    return o << "AABB(min = " << aabb.min << ", max = " << aabb.max << ')';
}

bool osc::operator==(AABB const& a, AABB const& b) noexcept
{
    return a.min == b.min && a.max == b.max;
}

bool osc::operator!=(AABB const& a, AABB const& b) noexcept
{
    return !(a == b);
}


// BVH implementation

// BVH helpers
namespace
{
    osc::AABB Union(nonstd::span<osc::BVHPrim const> prims)
    {
        osc::AABB rv = prims.front().getBounds();
        for (auto it = prims.begin() + 1; it != prims.end(); ++it)
        {
            rv = Union(rv, it->getBounds());
        }
        return rv;
    }

    template<typename T>
    T const& at(nonstd::span<T const> vs, size_t i)
    {
        if (i <= vs.size())
        {
            return vs[i];
        }
        else
        {
            throw std::out_of_range{"invalid span subscript"};
        }
    }

    bool HasAVolume(osc::Triangle const& t)
    {
        return !(t.p0 == t.p1 || t.p0 == t.p2 || t.p1 == t.p2);
    }

    // recursively build the BVH
    void BVH_RecursiveBuild(osc::BVH& bvh, ptrdiff_t const begin, ptrdiff_t const n)
    {
        if (n == 1)
        {
            // recursion bottomed out: create a leaf node
            bvh.nodes.push_back(osc::BVHNode::leaf(
                bvh.prims.at(begin).getBounds(),
                begin
            ));
            return;
        }

        // else: n >= 2, so partition the data appropriately and allocate an internal node

        ptrdiff_t midpoint = -1;
        ptrdiff_t internalNodeLoc = -1;
        {
            // allocate an appropriate internal node

            // compute bounding box of remaining (children) prims
            osc::AABB const aabb = Union({bvh.prims.data() + begin, static_cast<size_t>(n)});

            // compute slicing position along the longest dimension
            auto const longestDimIdx = LongestDimIndex(aabb);
            float const midpointX2 = aabb.min[longestDimIdx] + aabb.max[longestDimIdx];

            // returns true if a given primitive is below the midpoint along the dim
            auto const isBelowMidpoint = [longestDimIdx, midpointX2](osc::BVHPrim const& p)
            {
                float const primMidpointX2 = p.getBounds().min[longestDimIdx] + p.getBounds().max[longestDimIdx];
                return primMidpointX2 <= midpointX2;
            };

            // partition prims into above/below the midpoint
            ptrdiff_t const end = begin + n;
            auto const it = std::partition(bvh.prims.begin() + begin, bvh.prims.begin() + end, isBelowMidpoint);

            midpoint = std::distance(bvh.prims.begin(), it);
            if (midpoint == begin || midpoint == end)
            {
                // edge-case: failed to spacially partition: just naievely partition
                midpoint = begin + n/2;
            }

            internalNodeLoc = static_cast<ptrdiff_t>(bvh.nodes.size());

            // push the internal node
            bvh.nodes.push_back(osc::BVHNode::node(
                aabb,
                0  // the number of left-hand nodes is set later
            ));
        }

        // build left-hand subtree
        BVH_RecursiveBuild(bvh, begin, midpoint-begin);

        // the left-hand build allocated nodes for the left hand side contiguously in memory
        ptrdiff_t numLhsNodes = (static_cast<ptrdiff_t>(bvh.nodes.size()) - 1) - internalNodeLoc;
        OSC_ASSERT(numLhsNodes > 0);
        bvh.nodes[internalNodeLoc].setNumLhsNodes(numLhsNodes);

        // build right node
        BVH_RecursiveBuild(bvh, midpoint, (begin + n) - midpoint);
        OSC_ASSERT(internalNodeLoc+numLhsNodes < static_cast<ptrdiff_t>(bvh.nodes.size()));
    }

    // returns true if something hit (recursively)
    //
    // populates outparam with all AABB hits in depth-first order
    bool BVH_GetRayAABBCollisionsRecursive(
        osc::BVH const& bvh,
        osc::Line const& ray,
        ptrdiff_t nodeidx,
        std::vector<osc::BVHCollision>& out)
    {
        osc::BVHNode const& node = bvh.nodes[nodeidx];

        // check ray-AABB intersection with the BVH node
        std::optional<osc::RayCollision> res = osc::GetRayCollisionAABB(ray, node.getBounds());

        if (!res)
        {
            return false;  // no intersection with this node at all
        }

        if (node.isLeaf())
        {
            // it's a leaf node, so we've sucessfully found the AABB that intersected

            out.push_back(osc::BVHCollision{res->distance, res->position, bvh.prims[node.getFirstPrimOffset()].getID()});
            return true;
        }

        // else: we've "hit" an internal node and need to recurse to find the leaf
        bool const lhs = BVH_GetRayAABBCollisionsRecursive(bvh, ray, nodeidx+1, out);
        bool const rhs = BVH_GetRayAABBCollisionsRecursive(bvh, ray, nodeidx+node.getNumLhsNodes()+1, out);
        return lhs || rhs;
    }

    template<typename TIndex>
    std::optional<osc::BVHCollision> BVH_GetClosestRayIndexedTriangleCollisionRecursive(
        osc::BVH const& bvh,
        nonstd::span<glm::vec3 const> verts,
        nonstd::span<TIndex const> indices,
        osc::Line const& ray,
        float& closest,
        ptrdiff_t nodeidx)
    {
        osc::BVHNode const& node = bvh.nodes[nodeidx];
        std::optional<osc::RayCollision> const res = osc::GetRayCollisionAABB(ray, node.getBounds());

        if (!res)
        {
            return std::nullopt;  // didn't hit this node at all
        }

        if (res->distance > closest)
        {
            return std::nullopt;  // this AABB can't contain something closer
        }

        if (node.isLeaf())
        {
            // leaf node: check ray-triangle intersection

            osc::BVHPrim const& p = bvh.prims.at(node.getFirstPrimOffset());

            osc::Triangle const triangle =
            {
                at(verts, at(indices, p.getID())),
                at(verts, at(indices, p.getID()+1)),
                at(verts, at(indices, p.getID()+2)),
            };

            std::optional<osc::RayCollision> const rayTriangleColl = osc::GetRayCollisionTriangle(ray, triangle);

            if (rayTriangleColl && rayTriangleColl->distance < closest)
            {
                closest = rayTriangleColl->distance;
                return osc::BVHCollision{rayTriangleColl->distance, rayTriangleColl->position, p.getID()};
            }
            else
            {
                return std::nullopt;  // it didn't collide with the triangle
            }
        }

        // else: internal node: recurse
        std::optional<osc::BVHCollision> const lhs = BVH_GetClosestRayIndexedTriangleCollisionRecursive(bvh, verts, indices, ray, closest, nodeidx+1);
        std::optional<osc::BVHCollision> const rhs = BVH_GetClosestRayIndexedTriangleCollisionRecursive(bvh, verts, indices, ray, closest, nodeidx+node.getNumLhsNodes()+1);
        return rhs ? rhs : lhs;
    }

    template<typename TIndex>
    void BuildFromIndexedTriangles(
        osc::BVH& bvh,
        nonstd::span<glm::vec3 const> verts,
        nonstd::span<TIndex const> indices)
    {
        // clear out any old data
        bvh.clear();

        // build up the prim list for each triangle
        bvh.prims.reserve(indices.size()/3);  // good guess
        for (size_t i = 0; (i+2) < indices.size(); i += 3)
        {
            osc::Triangle const t
            {
                at(verts, indices[i]),
                at(verts, indices[i+1]),
                at(verts, indices[i+2]),
            };

            if (HasAVolume(t))
            {
                bvh.prims.emplace_back(static_cast<ptrdiff_t>(i), osc::AABBFromTriangle(t));
            }
        }

        if (!bvh.prims.empty())
        {
            BVH_RecursiveBuild(bvh, 0, static_cast<ptrdiff_t>(bvh.prims.size()));
        }
    }

    template<typename TIndex>
    std::optional<osc::BVHCollision> GetClosestRayIndexedTriangleCollision(
        osc::BVH const& bvh,
        nonstd::span<glm::vec3 const> verts,
        nonstd::span<TIndex const> indices,
        osc::Line const& ray)
    {
        if (bvh.nodes.empty() || bvh.prims.empty() || indices.empty())
        {
            return std::nullopt;
        }

        float closest = std::numeric_limits<float>::max();
        return BVH_GetClosestRayIndexedTriangleCollisionRecursive(bvh, verts, indices, ray, closest, 0);
    }
}

void osc::BVH::clear()
{
    nodes.clear();
    prims.clear();
}

void osc::BVH::buildFromIndexedTriangles(nonstd::span<glm::vec3 const> verts, nonstd::span<uint16_t const> indices)
{
    BuildFromIndexedTriangles<uint16_t>(*this, verts, indices);
}

void osc::BVH::buildFromIndexedTriangles(nonstd::span<glm::vec3 const> verts, nonstd::span<uint32_t const> indices)
{
    BuildFromIndexedTriangles<uint32_t>(*this, verts, indices);
}

std::optional<osc::BVHCollision> osc::BVH::getClosestRayIndexedTriangleCollision(nonstd::span<glm::vec3 const> verts, nonstd::span<uint16_t const> indices, Line const& line) const
{
    return GetClosestRayIndexedTriangleCollision<uint16_t>(*this, verts, indices, line);
}

std::optional<osc::BVHCollision> osc::BVH::getClosestRayIndexedTriangleCollision(nonstd::span<glm::vec3 const> verts, nonstd::span<uint32_t const> indices, Line const& line) const
{
    return GetClosestRayIndexedTriangleCollision<uint32_t>(*this, verts, indices, line);
}

void osc::BVH::buildFromAABBs(nonstd::span<AABB const> aabbs)
{
    // clear out any old data
    clear();

    // build up prim list for each AABB (just copy the AABB)
    prims.reserve(aabbs.size());  // good guess
    for (size_t i = 0; i < aabbs.size(); ++i)
    {
        if (!IsAPoint(aabbs[i]))
        {
            prims.emplace_back(static_cast<ptrdiff_t>(i), aabbs[i]);
        }
    }

    if (!prims.empty())
    {
        BVH_RecursiveBuild(*this, 0, static_cast<ptrdiff_t>(prims.size()));
    }
}

std::vector<osc::BVHCollision> osc::BVH::getRayAABBCollisions(Line const& ray) const
{
    std::vector<osc::BVHCollision> rv;

    if (nodes.empty())
    {
        return rv;
    }

    if (prims.empty())
    {
        return rv;
    }

    BVH_GetRayAABBCollisionsRecursive(*this, ray, 0, rv);

    return rv;
}

size_t osc::BVH::getMaxDepth() const
{
    size_t cur = 0;
    size_t maxdepth = 0;
    std::stack<size_t, std::vector<size_t>> stack;

    while (cur < nodes.size())
    {
        if (nodes[cur].isLeaf())
        {
            // leaf node: compute its depth and continue traversal (if applicable)

            maxdepth = std::max(maxdepth, stack.size() + 1);

            if (stack.empty())
            {
                break;  // nowhere to traverse to: exit
            }
            else
            {
                // traverse up to a parent node and try the right-hand side
                size_t const next = stack.top();
                stack.pop();
                cur = next + nodes[next].getNumLhsNodes() + 1;
            }
        }
        else
        {
            // internal node: push into the (right-hand) history stack and then
            //                traverse to the left-hand side

            stack.push(cur);
            cur += 1;
        }
    }

    return maxdepth;
}

std::optional<osc::AABB> osc::BVH::getRootAABB() const
{
    return !nodes.empty() ? nodes.front().getBounds() : std::optional<osc::AABB>{};
}


// osc::Disc implementation

std::ostream& osc::operator<<(std::ostream& o, Disc const& d)
{
    return o << "Disc(origin = " << d.origin << ", normal = " << d.normal << ", radius = " << d.radius << ')';
}


// osc::EulerPerspectiveCamera implementation

osc::EulerPerspectiveCamera::EulerPerspectiveCamera() :
    pos{0.0f, 0.0f, 0.0f},
    pitch{0.0f},
    yaw{-fpi/2.0f},
    fov{fpi * 70.0f/180.0f},
    znear{0.1f},
    zfar{1000.0f}
{
}

glm::vec3 osc::EulerPerspectiveCamera::getFront() const noexcept
{
    return glm::normalize(glm::vec3
    {
        std::cos(yaw) * std::cos(pitch),
        std::sin(pitch),
        std::sin(yaw) * std::cos(pitch),
    });
}

glm::vec3 osc::EulerPerspectiveCamera::getUp() const noexcept
{
    return glm::vec3{0.0f, 1.0f, 0.0f};
}

glm::vec3 osc::EulerPerspectiveCamera::getRight() const noexcept
{
    return glm::normalize(glm::cross(getFront(), getUp()));
}

glm::mat4 osc::EulerPerspectiveCamera::getViewMtx() const noexcept
{
    return glm::lookAt(pos, pos + getFront(), getUp());
}

glm::mat4 osc::EulerPerspectiveCamera::getProjMtx(float aspect_ratio) const noexcept
{
    return glm::perspective(fov, aspect_ratio, znear, zfar);
}


// osc::Line implementation

std::ostream& osc::operator<<(std::ostream& o, Line const& l)
{
    return o << "Line(origin = " << l.origin << ", direction = " << l.dir << ')';
}


// osc::Plane implementation

std::ostream& osc::operator<<(std::ostream& o, Plane const& p)
{
    return o << "Plane(origin = " << p.origin << ", normal = " << p.normal << ')';
}


// osc::PolarPerspectiveCamera implementation

namespace
{
    glm::vec3 PolarToCartesian(glm::vec3 focus, float radius, float theta, float phi)
    {
        float x = radius * std::sin(theta) * std::cos(phi);
        float y = radius * std::sin(phi);
        float z = radius * std::cos(theta) * std::cos(phi);

        return -focus + glm::vec3{x, y, z};
    }
}

osc::PolarPerspectiveCamera::PolarPerspectiveCamera() :
    radius{1.0f},
    theta{fpi4},
    phi{fpi4},
    focusPoint{0.0f, 0.0f, 0.0f},
    fov{120.0f},
    znear{0.1f},
    zfar{100.0f}
{
}

void osc::PolarPerspectiveCamera::reset()
{
    *this = {};
}

void osc::PolarPerspectiveCamera::pan(float aspectRatio, glm::vec2 delta) noexcept
{
    // how much panning is done depends on how far the camera is from the
    // origin (easy, with polar coordinates) *and* the FoV of the camera.
    float xAmt = delta.x * aspectRatio * (2.0f * std::tan(fov / 2.0f) * radius);
    float yAmt = -delta.y * (1.0f / aspectRatio) * (2.0f * std::tan(fov / 2.0f) * radius);

    // this assumes the scene is not rotated, so we need to rotate these
    // axes to match the scene's rotation
    glm::vec4 defaultPanningAx = {xAmt, yAmt, 0.0f, 1.0f};
    auto rotTheta = glm::rotate(glm::identity<glm::mat4>(), theta, glm::vec3{0.0f, 1.0f, 0.0f});
    auto thetaVec = glm::normalize(glm::vec3{std::sin(theta), 0.0f, std::cos(theta)});
    auto phiAxis = glm::cross(thetaVec, glm::vec3{0.0, 1.0f, 0.0f});
    auto rotPhi = glm::rotate(glm::identity<glm::mat4>(), phi, phiAxis);

    glm::vec4 panningAxes = rotPhi * rotTheta * defaultPanningAx;
    focusPoint.x += panningAxes.x;
    focusPoint.y += panningAxes.y;
    focusPoint.z += panningAxes.z;
}

void osc::PolarPerspectiveCamera::drag(glm::vec2 delta) noexcept
{
    theta += 2.0f * fpi * -delta.x;
    phi += 2.0f * fpi * delta.y;
}

void osc::PolarPerspectiveCamera::rescaleZNearAndZFarBasedOnRadius() noexcept
{
    // znear and zfar are only really dicated by the camera's radius, because
    // the radius is effectively the distance from the camera's focal point

    znear = 0.02f * radius;
    zfar = 20.0f * radius;
}

glm::mat4 osc::PolarPerspectiveCamera::getViewMtx() const noexcept
{
    // camera: at a fixed position pointing at a fixed origin. The "camera"
    // works by translating + rotating all objects around that origin. Rotation
    // is expressed as polar coordinates. Camera panning is represented as a
    // translation vector.

    // this maths is a complete shitshow and I apologize. It just happens to work for now. It's a polar coordinate
    // system that shifts the world based on the camera pan

    auto rotTheta = glm::rotate(glm::identity<glm::mat4>(), -theta, glm::vec3{0.0f, 1.0f, 0.0f});
    auto thetaVec = glm::normalize(glm::vec3{std::sin(theta), 0.0f, std::cos(theta)});
    auto phiAxis = glm::cross(thetaVec, glm::vec3{0.0, 1.0f, 0.0f});
    auto rotPhi = glm::rotate(glm::identity<glm::mat4>(), -phi, phiAxis);
    auto panTranslate = glm::translate(glm::identity<glm::mat4>(), focusPoint);
    return glm::lookAt(
        glm::vec3(0.0f, 0.0f, radius),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3{0.0f, 1.0f, 0.0f}) * rotTheta * rotPhi * panTranslate;
}

glm::mat4 osc::PolarPerspectiveCamera::getProjMtx(float aspectRatio) const noexcept
{
    return glm::perspective(fov, aspectRatio, znear, zfar);
}

glm::vec3 osc::PolarPerspectiveCamera::getPos() const noexcept
{
    return PolarToCartesian(focusPoint, radius, theta, phi);
}

glm::vec2 osc::PolarPerspectiveCamera::projectOntoScreenRect(glm::vec3 const& worldspaceLoc, Rect const& screenRect) const noexcept
{
    glm::vec2 dims = Dimensions(screenRect);
    glm::mat4 MV = getProjMtx(dims.x/dims.y) * getViewMtx();

    glm::vec4 ndc = MV * glm::vec4{worldspaceLoc, 1.0f};
    ndc /= ndc.w;  // perspective divide

    glm::vec2 ndc2D;
    ndc2D = {ndc.x, -ndc.y};        // [-1, 1], Y points down
    ndc2D += 1.0f;                  // [0, 2]
    ndc2D *= 0.5f;                  // [0, 1]
    ndc2D *= dims;                  // [0, w]
    ndc2D += screenRect.p1;         // [x, x + w]

    return ndc2D;
}

osc::Line osc::PolarPerspectiveCamera::unprojectTopLeftPosToWorldRay(glm::vec2 pos, glm::vec2 dims) const noexcept
{
    glm::mat4 const viewMatrix = getViewMtx();
    glm::mat4 const projectionMatrix = getProjMtx(dims.x/dims.y);

    return PerspectiveUnprojectTopLeftScreenPosToWorldRay(
        pos/dims,
        this->getPos(),
        viewMatrix,
        projectionMatrix
    );
}

bool osc::operator==(PolarPerspectiveCamera const& a, PolarPerspectiveCamera const& b) noexcept
{
    return
        a.radius == b.radius &&
        a.theta == b.theta &&
        a.phi == b.phi &&
        a.focusPoint == b.focusPoint &&
        a.fov == b.fov &&
        a.znear == b.znear &&
        a.zfar == b.zfar;
}

bool osc::operator!=(PolarPerspectiveCamera const& a, PolarPerspectiveCamera const& b) noexcept
{
    return !(a == b);
}

osc::PolarPerspectiveCamera osc::CreateCameraWithRadius(float r)
{
    PolarPerspectiveCamera rv;
    rv.radius = r;
    return rv;
}

osc::PolarPerspectiveCamera osc::CreateCameraFocusedOn(AABB const& aabb)
{
    osc::PolarPerspectiveCamera rv;
    osc::AutoFocus(rv, aabb);
    return rv;
}

glm::vec3 osc::RecommendedLightDirection(osc::PolarPerspectiveCamera const& c)
{
    // theta should track with the camera, so that the scene is always
    // illuminated from the viewer's perspective (#275)
    //
    // and the offset angle should try to closely match other GUIs, which tend to
    // light scenes right-to-left (almost +1 in Z, but slightly along -X also) - #590
    //
    // but don't offset this too much, because we are using double-sided normals
    // (#318, #168) and, if the camera is too angled relative to the PoV, it's
    // possible to see angled parts of the scene be illuminated from the back (which
    // should be impossible)
    float const theta = c.theta + fpi4/2.0f;

    // #549: phi shouldn't track with the camera, because changing the "height"/"slope"
    // of the camera with shadow rendering (#10) looks bizzare
    float const phi = fpi4;

    glm::vec3 const p = PolarToCartesian(c.focusPoint, c.radius, theta, phi);

    return glm::normalize(-c.focusPoint - p);
}

void osc::FocusAlongX(osc::PolarPerspectiveCamera& camera)
{
    camera.theta = osc::fpi2;
    camera.phi = 0.0f;
}

void osc::FocusAlongMinusX(osc::PolarPerspectiveCamera& camera)
{
    camera.theta = -osc::fpi2;
    camera.phi = 0.0f;
}

void osc::FocusAlongY(osc::PolarPerspectiveCamera& camera)
{
    camera.theta = 0.0f;
    camera.phi = osc::fpi2;
}

void osc::FocusAlongMinusY(osc::PolarPerspectiveCamera& camera)
{
    camera.theta = 0.0f;
    camera.phi = -osc::fpi2;
}

void osc::FocusAlongZ(osc::PolarPerspectiveCamera& camera)
{
    camera.theta = 0.0f;
    camera.phi = 0.0f;
}

void osc::FocusAlongMinusZ(osc::PolarPerspectiveCamera& camera)
{
    camera.theta = osc::fpi;
    camera.phi = 0.0f;
}

void osc::ZoomIn(osc::PolarPerspectiveCamera& camera)
{
    camera.radius *= 0.8f;
}

void osc::ZoomOut(osc::PolarPerspectiveCamera& camera)
{
    camera.radius *= 1.2f;
}

void osc::Reset(osc::PolarPerspectiveCamera& camera)
{
    camera = {};
    camera.theta = osc::fpi4;
    camera.phi = osc::fpi4;
}

void osc::AutoFocus(PolarPerspectiveCamera& camera, AABB const& elementAABB, float)
{
    Sphere const s = ToSphere(elementAABB);

    // auto-focus the camera with a minimum radius of 1m
    //
    // this will break autofocusing on very small models (e.g. insect legs) but
    // handles the edge-case of autofocusing an empty model (#552), which is a
    // more common use-case (e.g. for new users and users making human-sized models)
    camera.focusPoint = -s.origin;
    camera.radius = std::max(s.radius / std::tan(0.5f * camera.fov), 1.0f);
    camera.rescaleZNearAndZFarBasedOnRadius();
}


// osc::Rect implementation

std::ostream& osc::operator<<(std::ostream& o, Rect const& r)
{
    return o << "Rect(p1 = " << r.p1 << ", p2 = " << r.p2 << ")";
}

bool osc::operator==(Rect const& a, Rect const& b) noexcept
{
    return a.p1 == b.p1 && a.p2 == b.p2;
}

bool osc::operator!=(Rect const& a, Rect const& b) noexcept
{
    return !(a == b);
}


// osc::Segment implementation

std::ostream& osc::operator<<(std::ostream& o, Segment const& d)
{
    return o << "Segment(p1 = " << d.p1 << ", p2 = " << d.p2 << ')';
}


// osc::Sphere implementation

std::ostream& osc::operator<<(std::ostream& o, Sphere const& s)
{
    return o << "Sphere(origin = " << s.origin << ", radius = " << s.radius << ')';
}


// osc::Tetrahedron implementatioon

// returns the volume of a given tetrahedron, defined as 4 points in space
float osc::Volume(Tetrahedron const& t)
{
    // sources:
    //
    // http://forums.cgsociety.org/t/how-to-calculate-center-of-mass-for-triangular-mesh/1309966
    // https://stackoverflow.com/questions/9866452/calculate-volume-of-any-tetrahedron-given-4-points

    glm::mat4 const m
    {
        glm::vec4{t[0], 1.0f},
        glm::vec4{t[1], 1.0f},
        glm::vec4{t[2], 1.0f},
        glm::vec4{t[3], 1.0f},
    };

    return glm::determinant(m) / 6.0f;
}

// returns spatial centerpoint of a given tetrahedron
glm::vec3 osc::Center(Tetrahedron const& t)
{
    // arithmetic mean of tetrahedron vertices
    return std::reduce(t.begin(), t.end()) / static_cast<float>(t.size());
}


// osc::Transform implementation

std::ostream& osc::operator<<(std::ostream& o, Transform const& t)
{
    return o << "Transform(position = " << t.position << ", rotation = " << t.rotation << ", scale = " << t.scale << ')';
}

bool osc::operator==(osc::Transform const& a, osc::Transform const& b) noexcept
{
    return
        a.scale == b.scale &&
        a.rotation == b.rotation &&
        a.position == b.position;
}

glm::vec3 osc::operator*(Transform const& t, glm::vec3 const& p) noexcept
{
    return TransformPoint(t, p);
}

osc::Transform& osc::operator+=(Transform& t, Transform const& o) noexcept
{
    t.position += o.position;
    t.rotation += o.rotation;
    t.scale += o.scale;
    return t;
}

osc::Transform& osc::operator/=(Transform& t, float s) noexcept
{
    t.position /= s;
    t.rotation /= s;
    t.scale /= s;
    return t;
}


// Geometry implementation


namespace
{
    struct QuadraticFormulaResult final {
        bool computeable;
        float x0;
        float x1;
    };

    // solve a quadratic formula
    //
    // only real-valued results supported - no complex-plane results
    QuadraticFormulaResult solveQuadratic(float a, float b, float c)
    {
        QuadraticFormulaResult res;

        // b2 - 4ac
        float const discriminant = b*b - 4.0f*a*c;

        if (discriminant < 0.0f)
        {
            res.computeable = false;
            return res;
        }

        // q = -1/2 * (b +- sqrt(b2 - 4ac))
        float const q = -0.5f * (b + std::copysign(std::sqrt(discriminant), b));

        // you might be wondering why this doesn't just compute a textbook
        // version of the quadratic equation (-b +- sqrt(disc))/2a
        //
        // the reason is because `-b +- sqrt(b2 - 4ac)` can result in catastrophic
        // cancellation if `-b` is close to `sqrt(disc)`
        //
        // so, instead, we use two similar, complementing, quadratics:
        //
        // the textbook one:
        //
        //     x = (-b +- sqrt(disc)) / 2a
        //
        // and the "Muller's method" one:
        //
        //     x = 2c / (-b -+ sqrt(disc))
        //
        // the great thing about these two is that the "+-" part of their
        // equations are complements, so you can have:
        //
        // q = -0.5 * (b + sign(b)*sqrt(disc))
        //
        // which, handily, will only *accumulate* the sum inside those
        // parentheses. If `b` is positive, you end up with a positive
        // number. If `b` is negative, you end up with a negative number. No
        // catastropic cancellation. By multiplying it by "-0.5" you end up
        // with:
        //
        //     -b - sqrt(disc)
        //
        // or, if B was negative:
        //
        //     -b + sqrt(disc)
        //
        // both of which are valid terms of both the quadratic equations above
        //
        // see:
        //
        //     https://math.stackexchange.com/questions/1340267/alternative-quadratic-formula
        //     https://en.wikipedia.org/wiki/Quadratic_equation


        res.computeable = true;
        res.x0 = q/a;  // textbook "complete the square" equation
        res.x1 = c/q;  // Muller's method equation
        return res;
    }

    std::optional<osc::RayCollision> GetRayCollisionSphere(osc::Sphere const& s, osc::Line const& l) noexcept
    {
        // see: https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-sphere-intersection

        glm::vec3 L = s.origin - l.origin;  // line origin to sphere origin
        float tca = glm::dot(L, l.dir);  // projected line from middle of hitline to sphere origin

        if (tca < 0.0f)
        {
            // line is pointing away from the sphere
            return std::nullopt;
        }

        float d2 = glm::dot(L, L) - tca*tca;
        float r2 = s.radius * s.radius;

        if (d2 > r2)
        {
            // line is not within the sphere's radius
            return std::nullopt;
        }

        // the collision points are on the sphere's surface (R), and D
        // is how far the hitline midpoint is from the radius. Can use
        // Pythag to figure out the midpoint length (thc)
        float thc = glm::sqrt(r2 - d2);
        float distance = tca - thc;

        return osc::RayCollision{distance, l.origin + distance*l.dir};
    }

    std::optional<osc::RayCollision> GetRayCollisionSphereAnalytic(osc::Sphere const& s, osc::Line const& l) noexcept
    {
        // see: https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-sphere-intersection

        glm::vec3 L = l.origin - s.origin;

        // coefficients of the quadratic implicit:
        //
        //     P2 - R2 = 0
        //     (O + tD)2 - R2 = 0
        //     (O + tD - C)2 - R2 = 0
        //
        // where:
        //
        //     P    a point on the surface of the sphere
        //     R    the radius of the sphere
        //     O    origin of line
        //     t    scaling factor for line direction (we want this)
        //     D    direction of line
        //     C    center of sphere
        //
        // if the quadratic has solutions, then there must exist one or two
        // `t`s that are points on the sphere's surface.

        float a = glm::dot(l.dir, l.dir);  // always == 1.0f if d is normalized
        float b = 2.0f * glm::dot(l.dir, L);
        float c = glm::dot(L, L) - glm::dot(s.radius, s.radius);

        auto [ok, t0, t1] = solveQuadratic(a, b, c);

        if (!ok)
        {
            return std::nullopt;
        }

        // ensure X0 < X1
        if (t0 > t1)
        {
            std::swap(t0, t1);
        }

        // ensure it's in front
        if (t0 < 0.0f)
        {
            t0 = t1;
            if (t0 < 0.0f)
            {
                return std::nullopt;
            }
        }

        return osc::RayCollision{t0, l.origin + t0*l.dir};
    }
}


// MathHelpers

// returns `true` if the values of `a` and `b` are effectively equal
//
// this algorithm is designed to be correct, rather than fast
bool osc::IsEffectivelyEqual(double a, double b) noexcept
{
    // why:
    //
    //     http://realtimecollisiondetection.net/blog/?p=89
    //     https://stackoverflow.com/questions/17333/what-is-the-most-effective-way-for-float-and-double-comparison
    //
    // machine epsilon is only relevant for numbers < 1.0, so the epsilon
    // value must be scaled up to the magnitude of the operands if you need
    // a more-correct equality comparison

    double const scaledEpsilon = std::max({1.0, a, b}) * std::numeric_limits<double>::epsilon();
    return std::abs(a - b) < scaledEpsilon;
}

bool osc::IsLessThanOrEffectivelyEqual(double a, double b) noexcept
{
    if (a <= b)
    {
        return true;
    }
    else
    {
        return IsEffectivelyEqual(a, b);
    }
}

bool osc::AreAtSameLocation(glm::vec3 const& a, glm::vec3 const& b) noexcept
{
    constexpr float eps2 = std::numeric_limits<float>::epsilon() * std::numeric_limits<float>::epsilon();

    glm::vec3 const b2a = a - b;
    float const len2 = glm::dot(b2a, b2a);

    return len2 > eps2;
}

glm::vec3 osc::Min(glm::vec3 const& a, glm::vec3 const& b) noexcept
{
    return glm::vec3
    {
        std::min(a.x, b.x),
        std::min(a.y, b.y),
        std::min(a.z, b.z),
    };
}

glm::vec2 osc::Min(glm::vec2 const& a, glm::vec2 const& b) noexcept
{
    return glm::vec2
    {
        std::min(a.x, b.x),
        std::min(a.y, b.y),
    };
}

glm::ivec2 osc::Min(glm::ivec2 const& a, glm::ivec2 const& b) noexcept
{
    return glm::ivec2
    {
        std::min(a.x, b.x),
        std::min(a.y, b.y),
    };
}

glm::vec3 osc::Max(glm::vec3 const& a, glm::vec3 const& b) noexcept
{
    return glm::vec3
    {
        std::max(a.x, b.x),
        std::max(a.y, b.y),
        std::max(a.z, b.z),
    };
}

glm::vec2 osc::Max(glm::vec2 const& a, glm::vec2 const& b) noexcept
{
    return glm::vec2
    {
        std::max(a.x, b.x),
        std::max(a.y, b.y),
    };
}

glm::ivec2 osc::Max(glm::ivec2 const& a, glm::ivec2 const& b) noexcept
{
    return glm::ivec2
    {
        std::max(a.x, b.x),
        std::max(a.y, b.y),
    };
}

glm::vec3::length_type osc::LongestDimIndex(glm::vec3 const& v) noexcept
{
    if (v.x > v.y && v.x > v.z)
    {
        return 0;  // X is longest
    }
    else if (v.y > v.z)
    {
        return 1;  // Y is longest
    }
    else
    {
        return 2;  // Z is longest
    }
}

glm::vec2::length_type osc::LongestDimIndex(glm::vec2 v) noexcept
{
    if (v.x > v.y)
    {
        return 0;  // X is longest
    }
    else
    {
        return 1;  // Y is longest
    }
}

glm::ivec2::length_type osc::LongestDimIndex(glm::ivec2 v) noexcept
{
    if (v.x > v.y)
    {
        return 0;  // X is longest
    }
    else
    {
        return 1;  // Y is longest
    }
}

float osc::LongestDim(glm::vec3 const& v) noexcept
{
    return v[LongestDimIndex(v)];
}

float osc::LongestDim(glm::vec2 v) noexcept
{
    return v[LongestDimIndex(v)];
}

glm::ivec2::value_type osc::LongestDim(glm::ivec2 v) noexcept
{
    return v[LongestDimIndex(v)];
}

float osc::AspectRatio(glm::ivec2 v) noexcept
{
    return static_cast<float>(v.x) / static_cast<float>(v.y);
}

float osc::AspectRatio(glm::vec2 v) noexcept
{
    return v.x/v.y;
}

glm::vec2 osc::Midpoint(glm::vec2 a, glm::vec2 b) noexcept
{
    return 0.5f*(a+b);
}

glm::vec3 osc::Midpoint(glm::vec3 const& a, glm::vec3 const& b) noexcept
{
    return 0.5f*(a+b);
}

glm::vec3 osc::Midpoint(nonstd::span<glm::vec3 const> vs) noexcept
{
    return std::reduce(vs.begin(), vs.end()) / static_cast<float>(vs.size());
}


// Geometry

glm::vec3 osc::KahanSum(nonstd::span<glm::vec3 const> vs) noexcept
{
    glm::vec3 sum{};  // accumulator
    glm::vec3 c{};    // running compensation of low-order bits

    for (size_t i = 0; i < vs.size(); ++i)
    {
        glm::vec3 y = vs[i] - c;  // subtract the compensation amount from the next number
        glm::vec3 t = sum + y;    // perform the summation (might lose information)
        c = (t - sum) - y;        // (t-sum) yields the retained (high-order) parts of `y`, so `c` contains the "lost" information
        sum = t;                  // CAREFUL: algebreically, `c` always == 0 - despite the computer's (actual) limited precision, the compiler might elilde all of this
    }

    return sum;
}

glm::vec3 osc::NumericallyStableAverage(nonstd::span<glm::vec3 const> vs) noexcept
{
    glm::vec3 const sum = KahanSum(vs);
    return sum / static_cast<float>(vs.size());
}

glm::vec3 osc::TriangleNormal(Triangle const& tri) noexcept
{
    glm::vec3 const ab = tri.p1 - tri.p0;
    glm::vec3 const ac = tri.p2 - tri.p0;
    glm::vec3 const perpendiular = glm::cross(ab, ac);
    return glm::normalize(perpendiular);
}

glm::mat3 osc::ToAdjugateMatrix(glm::mat3 const& m) noexcept
{
    // google: "Adjugate Matrix": it's related to the cofactor matrix and is
    // related to the inverse of a matrix through:
    //
    //     inverse(M) = Adjugate(M) / determinant(M);

    glm::mat3 rv;
    rv[0][0] = + (m[1][1] * m[2][2] - m[2][1] * m[1][2]);
    rv[1][0] = - (m[1][0] * m[2][2] - m[2][0] * m[1][2]);
    rv[2][0] = + (m[1][0] * m[2][1] - m[2][0] * m[1][1]);
    rv[0][1] = - (m[0][1] * m[2][2] - m[2][1] * m[0][2]);
    rv[1][1] = + (m[0][0] * m[2][2] - m[2][0] * m[0][2]);
    rv[2][1] = - (m[0][0] * m[2][1] - m[2][0] * m[0][1]);
    rv[0][2] = + (m[0][1] * m[1][2] - m[1][1] * m[0][2]);
    rv[1][2] = - (m[0][0] * m[1][2] - m[1][0] * m[0][2]);
    rv[2][2] = + (m[0][0] * m[1][1] - m[1][0] * m[0][1]);
    return rv;
}

glm::mat3 osc::ToNormalMatrix(glm::mat4 const& m) noexcept
{
    // "On the Transformation of Surface Normals" by Andrew Glassner (1987)
    //
    // "One option is to replace the inverse with the adjoint of M. The
    //  adjoint is attractive because it always exists, even when M is
    //  singular. The inverse and the adjoint are related by:
    //
    //      inverse(M) = adjoint(M) / determinant(M);
    //
    //  so, when the inverse exists, they only differ by a constant factor.
    //  Therefore, using adjoint(M) instead of inverse(M) only affects the
    //  magnitude of the resulting normal vector. Normal vectors have to
    //  be normalized after mutiplication with a normal matrix anyway, so
    //  nothing is lost"

    glm::mat3 const topLeft{m};
    return ToAdjugateMatrix(glm::transpose(topLeft));
}

glm::mat3 osc::ToNormalMatrix(glm::mat4x3 const& m) noexcept
{
    // "On the Transformation of Surface Normals" by Andrew Glassner (1987)
    //
    // "One option is to replace the inverse with the adjoint of M. The
    //  adjoint is attractive because it always exists, even when M is
    //  singular. The inverse and the adjoint are related by:
    //
    //      inverse(M) = adjoint(M) / determinant(M);
    //
    //  so, when the inverse exists, they only differ by a constant factor.
    //  Therefore, using adjoint(M) instead of inverse(M) only affects the
    //  magnitude of the resulting normal vector. Normal vectors have to
    //  be normalized after mutiplication with a normal matrix anyway, so
    //  nothing is lost"

    glm::mat3 const topLeft{m};
    return ToAdjugateMatrix(glm::transpose(topLeft));
}

glm::mat4 osc::ToNormalMatrix4(glm::mat4 const& m) noexcept
{
    return ToNormalMatrix(m);
}

glm::mat4 osc::Dir1ToDir2Xform(glm::vec3 const& a, glm::vec3 const& b) noexcept
{
    // this is effectively a rewrite of glm::rotation(vec3 const&, vec3 const& dest);

    float const cosTheta = glm::dot(a, b);

    if(cosTheta >= static_cast<float>(1.0f) - std::numeric_limits<float>::epsilon())
    {
        // `a` and `b` point in the same direction: return identity transform
        return glm::mat4{1.0f};
    }

    float theta;
    glm::vec3 rotationAxis;
    if(cosTheta < static_cast<float>(-1.0f) + std::numeric_limits<float>::epsilon())
    {
        // `a` and `b` point in opposite directions
        //
        // - there is no "ideal" rotation axis
        // - so we try "guessing" one and hope it's good (then try another if it isn't)

        rotationAxis = glm::cross(glm::vec3{0.0f, 0.0f, 1.0f}, a);
        if (glm::length2(rotationAxis) < std::numeric_limits<float>::epsilon())
        {
            // bad luck: they were parallel - use a different axis
            rotationAxis = glm::cross(glm::vec3{1.0f, 0.0f, 0.0f}, a);
        }

        theta = osc::fpi;
        rotationAxis = glm::normalize(rotationAxis);
    }
    else
    {
        theta = glm::acos(cosTheta);
        rotationAxis = glm::normalize(glm::cross(a, b));
    }

    return glm::rotate(glm::mat4{1.0f}, theta, rotationAxis);
}

glm::vec3 osc::ExtractEulerAngleXYZ(glm::quat const& q) noexcept
{
    glm::vec3 rv;
    glm::extractEulerAngleXYZ(glm::toMat4(q), rv.x, rv.y, rv.z);
    return rv;
}

glm::vec3 osc::ExtractEulerAngleXYZ(glm::mat4 const& m) noexcept
{
    glm::vec3 v;
    glm::extractEulerAngleXYZ(m, v.x, v.y, v.z);
    return v;
}

glm::vec2 osc::TopleftRelPosToNDCPoint(glm::vec2 p)
{
    p.y = 1.0f - p.y;
    return 2.0f*p - 1.0f;
}

glm::vec2 osc::NDCPointToTopLeftRelPos(glm::vec2 p)
{
    p = (p + 1.0f) * 0.5f;
    p.y = 1.0f - p.y;
    return p;
}

glm::vec4 osc::TopleftRelPosToNDCCube(glm::vec2 relpos)
{
    return {TopleftRelPosToNDCPoint(relpos), -1.0f, 1.0f};
}

osc::Line osc::PerspectiveUnprojectTopLeftScreenPosToWorldRay(
    glm::vec2 relpos,
    glm::vec3 cameraWorldspaceLocation,
    glm::mat4 const& viewMatrix,
    glm::mat4 const& projMatrix)
{
    // position of point, as if it were on the front of the 3D NDC cube
    glm::vec4 lineOriginNDC = TopleftRelPosToNDCCube(relpos);

    glm::vec4 lineOriginView = glm::inverse(projMatrix) * lineOriginNDC;
    lineOriginView /= lineOriginView.w;  // perspective divide

    // location of mouse in worldspace
    glm::vec3 lineOriginWorld = glm::vec3{glm::inverse(viewMatrix) * lineOriginView};

    // direction vector from camera to mouse location (i.e. the projection)
    glm::vec3 lineDirWorld = glm::normalize(lineOriginWorld - cameraWorldspaceLocation);

    Line rv;
    rv.dir = lineDirWorld;
    rv.origin = lineOriginWorld;
    return rv;
}

glm::vec2 osc::MinValuePerDimension(Rect const& r) noexcept
{
    return Min(r.p1, r.p2);
}

float osc::Area(Rect const& r) noexcept
{
    auto d = Dimensions(r);
    return d.x * d.y;
}

glm::vec2 osc::Dimensions(Rect const& r) noexcept
{
    return glm::abs(r.p2 - r.p1);
}

glm::vec2 osc::BottomLeft(Rect const& r) noexcept
{
    return glm::vec2{glm::min(r.p1.x, r.p2.x), glm::max(r.p1.y, r.p2.y)};
}

float osc::AspectRatio(Rect const& r) noexcept
{
    glm::vec2 dims = Dimensions(r);
    return dims.x/dims.y;
}

glm::vec2 osc::Midpoint(Rect const& r) noexcept
{
    return 0.5f * (r.p1 + r.p2);
}

osc::Rect osc::Expand(Rect const& rect, float amt) noexcept
{
    Rect rv
    {
        Min(rect.p1, rect.p2),
        Max(rect.p1, rect.p2)
    };
    rv.p1.x -= amt;
    rv.p2.x += amt;
    rv.p1.y -= amt;
    rv.p2.y += amt;
    return rv;
}

osc::Rect osc::Expand(Rect const& rect, glm::vec2 amt) noexcept
{
    Rect rv
    {
        Min(rect.p1, rect.p2),
        Max(rect.p1, rect.p2)
    };
    rv.p1.x -= amt.x;
    rv.p2.x += amt.x;
    rv.p1.y -= amt.y;
    rv.p2.y += amt.y;
    return rv;
}

osc::Rect osc::Clamp(Rect const& r, glm::vec2 const& min, glm::vec2 const& max) noexcept
{
    return
    {
        glm::clamp(r.p1, min, max),
        glm::clamp(r.p2, min, max),
    };
}

osc::Rect osc::NdcRectToScreenspaceViewportRect(Rect const& ndcRect, Rect const& viewport) noexcept
{
    glm::vec2 const viewportDims = Dimensions(viewport);

    // remap [-1, 1] into [0, viewportDims]
    Rect rv
    {
        0.5f * (ndcRect.p1 + 1.0f) * viewportDims,
        0.5f * (ndcRect.p2 + 1.0f) * viewportDims,
    };

    // offset by viewport's top-left
    rv.p1 += viewport.p1;
    rv.p2 += viewport.p1;

    return rv;
}

osc::Sphere osc::BoundingSphereOf(nonstd::span<glm::vec3 const> points) noexcept
{
    AABB const aabb = AABBFromVerts(points);

    Sphere rv{};
    rv.origin = Midpoint(aabb);
    rv.radius = 0.0f;

    // edge-case: no points provided
    if (points.size() == 0)
    {
        return rv;
    }

    float biggestR2 = 0.0f;
    for (glm::vec3 const& pos : points)
    {
        glm::vec3 pos2rv = pos - rv.origin;
        float r2 = glm::dot(pos2rv, pos2rv);
        biggestR2 = std::max(biggestR2, r2);
    }

    rv.radius = glm::sqrt(biggestR2);

    return rv;
}

osc::Sphere osc::ToSphere(AABB const& aabb) noexcept
{
    return BoundingSphereOf(ToCubeVerts(aabb));
}

glm::mat4 osc::FromUnitSphereMat4(Sphere const& s) noexcept
{
    return glm::translate(glm::mat4{1.0f}, s.origin) * glm::scale(glm::mat4{1.0f}, {s.radius, s.radius, s.radius});
}

glm::mat4 osc::SphereToSphereMat4(Sphere const& a, Sphere const& b) noexcept
{
    float scale = b.radius/a.radius;
    glm::mat4 scaler = glm::scale(glm::mat4{1.0f}, glm::vec3{scale, scale, scale});
    glm::mat4 mover = glm::translate(glm::mat4{1.0f}, b.origin - a.origin);
    return mover * scaler;
}

osc::Transform osc::SphereToSphereTransform(Sphere const& a, Sphere const& b) noexcept
{
    Transform t;
    t.scale *= (b.radius / a.radius);
    t.position = b.origin - a.origin;
    return t;
}

osc::AABB osc::ToAABB(Sphere const& s) noexcept
{
    AABB rv;
    rv.min = s.origin - s.radius;
    rv.max = s.origin + s.radius;
    return rv;
}

osc::Line osc::TransformLine(Line const& l, glm::mat4 const& m) noexcept
{
    Line rv;
    rv.dir = m * glm::vec4{l.dir, 0.0f};
    rv.origin = m * glm::vec4{l.origin, 1.0f};
    return rv;
}

osc::Line osc::InverseTransformLine(Line const& l, Transform const& t) noexcept
{
    return Line
    {
        InverseTransformPoint(t, l.origin),
        InverseTransformDirection(t, l.dir),
    };
}

glm::mat4 osc::DiscToDiscMat4(Disc const& a, Disc const& b) noexcept
{
    // this is essentially LERPing [0,1] onto [1, l] to rescale only
    // along the line's original direction

    // scale factor
    float s = b.radius/a.radius;

    // LERP the axes as follows
    //
    // - 1.0f if parallel with N
    // - s if perpendicular to N
    // - N is a directional vector, so it's `cos(theta)` in each axis already
    // - 1-N is sin(theta) of each axis to the normal
    // - LERP is 1.0f + (s - 1.0f)*V, where V is how perpendiular each axis is

    glm::vec3 scalers = 1.0f + ((s - 1.0f) * glm::abs(1.0f - a.normal));
    glm::mat4 scaler = glm::scale(glm::mat4{1.0f}, scalers);

    float cosTheta = glm::dot(a.normal, b.normal);
    glm::mat4 rotator;
    if (cosTheta > 0.9999f)
    {
        rotator = glm::mat4{1.0f};
    }
    else
    {
        float theta = glm::acos(cosTheta);
        glm::vec3 axis = glm::cross(a.normal, b.normal);
        rotator = glm::rotate(glm::mat4{1.0f}, theta, axis);
    }

    glm::mat4 translator = glm::translate(glm::mat4{1.0f}, b.origin-a.origin);

    return translator * rotator * scaler;
}

osc::AABB osc::InvertedAABB() noexcept
{
    AABB rv;
    rv.min = {std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max()};
    rv.max = {std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest()};
    return rv;
}

glm::vec3 osc::Midpoint(AABB const& a) noexcept
{
    return 0.5f * (a.min + a.max);
}

glm::vec3 osc::Dimensions(AABB const& a) noexcept
{
    return a.max - a.min;
}

float osc::Volume(AABB const& a) noexcept
{
    glm::vec3 d = Dimensions(a);
    return d.x * d.y * d.z;
}

osc::AABB osc::Union(AABB const& a, AABB const& b) noexcept
{
    return AABB
    {
        Min(a.min, b.min),
        Max(a.max, b.max),
    };
}

bool osc::IsAPoint(AABB const& a) noexcept
{
    return a.min == a.max;
}

bool osc::IsZeroVolume(AABB const& a) noexcept
{

    for (glm::vec3::length_type i = 0; i < 3; ++i)
    {
        if (a.min[i] == a.max[i])
        {
            return true;
        }
    }
    return false;
}

glm::vec3::length_type osc::LongestDimIndex(AABB const& a) noexcept
{
    return LongestDimIndex(Dimensions(a));
}

float osc::LongestDim(AABB const& a) noexcept
{
    glm::vec3 dims = Dimensions(a);
    float rv = dims[0];
    rv = std::max(rv, dims[1]);
    rv = std::max(rv, dims[2]);
    return rv;
}

std::array<glm::vec3, 8> osc::ToCubeVerts(AABB const& aabb) noexcept
{
    glm::vec3 d = Dimensions(aabb);

    std::array<glm::vec3, 8> rv{};
    rv[0] = aabb.min;
    rv[1] = aabb.max;
    size_t pos = 2;
    for (glm::vec3::length_type i = 0; i < 3; ++i)
    {
        glm::vec3 min = aabb.min;
        min[i] += d[i];
        glm::vec3 max = aabb.max;
        max[i] -= d[i];
        rv[pos++] = min;
        rv[pos++] = max;
    }
    return rv;
}

osc::AABB osc::TransformAABB(AABB const& aabb, glm::mat4 const& m) noexcept
{
    auto verts = ToCubeVerts(aabb);

    for (glm::vec3& vert : verts)
    {
        glm::vec4 p = m * glm::vec4{vert, 1.0f};
        vert = glm::vec3{p / p.w}; // perspective divide
    }

    return AABBFromVerts(verts);
}

osc::AABB osc::TransformAABB(AABB const& aabb, Transform const& t) noexcept
{
    // from real-time collision detection (the book)
    //
    // screenshot: https://twitter.com/Herschel/status/1188613724665335808

    glm::mat3 const m = ToMat3(t);

    AABB rv{t.position, t.position};  // add in the translation
    for (glm::vec3::length_type i = 0; i < 3; ++i)
    {
        // form extent by summing smaller and larger terms repsectively
        for (glm::vec3::length_type j = 0; j < 3; ++j)
        {
            float const e = m[j][i] * aabb.min[j];
            float const f = m[j][i] * aabb.max[j];

            if (e < f)
            {
                rv.min[i] += e;
                rv.max[i] += f;
            }
            else
            {
                rv.min[i] += f;
                rv.max[i] += e;
            }
        }
    }
    return rv;
}

osc::AABB osc::AABBFromTriangle(Triangle const& t) noexcept
{
    AABB rv{t.p0, t.p0};
    rv.min = Min(rv.min, t.p1);
    rv.max = Max(rv.max, t.p1);
    rv.min = Min(rv.min, t.p2);
    rv.max = Max(rv.max, t.p2);
    return rv;
}

osc::AABB osc::AABBFromVerts(nonstd::span<glm::vec3 const> vs) noexcept
{
    // edge-case: no points provided
    if (vs.empty())
    {
        return AABB{};
    }

    // otherwise, compute bounds
    AABB rv{vs[0], vs[0]};
    for (size_t i = 1; i < vs.size(); ++i)
    {
        glm::vec3 const& pos = vs[i];
        rv.min = Min(rv.min, pos);
        rv.max = Max(rv.max, pos);
    }

    return rv;
}

osc::AABB osc::AABBFromIndexedVerts(
    nonstd::span<glm::vec3 const> verts,
    nonstd::span<uint32_t const> indices)
{
    AABB rv{};

    // edge-case: no points provided
    if (indices.empty())
    {
        return rv;
    }

    rv.min =
    {
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max(),
    };

    rv.max =
    {
        std::numeric_limits<float>::lowest(),
        std::numeric_limits<float>::lowest(),
        std::numeric_limits<float>::lowest(),
    };

    for (uint32_t idx : indices)
    {
        if (idx < verts.size())  // ignore invalid indices
        {
            glm::vec3 const& pos = verts[idx];
            rv.min = Min(rv.min, pos);
            rv.max = Max(rv.max, pos);
        }
    }

    return rv;
}

osc::AABB osc::AABBFromIndexedVerts(
    nonstd::span<glm::vec3 const> verts,
    nonstd::span<uint16_t const> indices)
{
    AABB rv{};

    // edge-case: no points provided
    if (indices.empty())
    {
        return rv;
    }

    rv.min =
    {
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max(),
    };

    rv.max =
    {
        std::numeric_limits<float>::lowest(),
        std::numeric_limits<float>::lowest(),
        std::numeric_limits<float>::lowest(),
    };

    for (uint16_t idx : indices)
    {
        if (idx < verts.size())  // ignore invalid indices
        {
            glm::vec3 const& pos = verts[idx];
            rv.min = Min(rv.min, pos);
            rv.max = Max(rv.max, pos);
        }
    }

    return rv;
}

std::optional<osc::Rect> osc::AABBToScreenNDCRect(
    AABB const& aabb,
    glm::mat4 const& viewMat,
    glm::mat4 const& projMat,
    float znear,
    float zfar)
{
    // create a new AABB in viewspace that bounds the worldspace AABB
    AABB viewspaceAABB = TransformAABB(aabb, viewMat);

    // z-test the viewspace AABB to see if any part of it it falls within the
    // camera's clipping planes
    //
    // care: znear and zfar are usually defined as positive distances from the
    //       camera but viewspace points along -Z

    if (viewspaceAABB.min.z > -znear && viewspaceAABB.max.z > -znear)
    {
        return std::nullopt;
    }
    if (viewspaceAABB.min.z < -zfar && viewspaceAABB.max.z < -zfar)
    {
        return std::nullopt;
    }

    // clamp the viewspace AABB to within the camera's clipping planes
    viewspaceAABB.min.z = glm::clamp(viewspaceAABB.min.z, -zfar, -znear);
    viewspaceAABB.max.z = glm::clamp(viewspaceAABB.max.z, -zfar, -znear);

    // transform it into an NDC-aligned NDC-space AABB
    AABB ndcAABB = TransformAABB(viewspaceAABB, projMat);

    // take the X and Y coordinates of that AABB and ensure they are clamped to within bounds
    Rect rv{glm::vec2{ndcAABB.min}, glm::vec2{ndcAABB.max}};
    rv.p1 = glm::clamp(rv.p1, {-1.0f, -1.0f}, {1.0f, 1.0f});
    rv.p2 = glm::clamp(rv.p2, { -1.0f, -1.0f }, {1.0f, 1.0f});

    return rv;
}

glm::mat4 osc::SegmentToSegmentMat4(Segment const& a, Segment const& b) noexcept
{
    glm::vec3 a1ToA2 = a.p2 - a.p1;
    glm::vec3 b1ToB2 = b.p2 - b.p1;

    float aLen = glm::length(a1ToA2);
    float bLen = glm::length(b1ToB2);

    glm::vec3 aDir = a1ToA2/aLen;
    glm::vec3 bDir = b1ToB2/bLen;

    glm::vec3 aCenter = (a.p1 + a.p2)/2.0f;
    glm::vec3 bCenter = (b.p1 + b.p2)/2.0f;

    // this is essentially LERPing [0,1] onto [1, l] to rescale only
    // along the line's original direction
    float s = bLen/aLen;
    glm::vec3 scaler = glm::vec3{1.0f, 1.0f, 1.0f} + (s-1.0f)*aDir;

    glm::mat4 rotate = Dir1ToDir2Xform(aDir, bDir);
    glm::mat4 scale = glm::scale(glm::mat4{1.0f}, scaler);
    glm::mat4 move = glm::translate(glm::mat4{1.0f}, bCenter - aCenter);

    return move * rotate * scale;
}

osc::Transform osc::SegmentToSegmentTransform(Segment const& a, Segment const& b) noexcept
{
    glm::vec3 aLine = a.p2 - a.p1;
    glm::vec3 bLine = b.p2 - b.p1;

    float aLen = glm::length(aLine);
    float bLen = glm::length(bLine);

    glm::vec3 aDir = aLine/aLen;
    glm::vec3 bDir = bLine/bLen;

    glm::vec3 aMid = (a.p1 + a.p2)/2.0f;
    glm::vec3 bMid = (b.p1 + b.p2)/2.0f;

    // for scale: LERP [0,1] onto [1,l] along original direction
    Transform t;
    t.rotation = glm::rotation(aDir, bDir);
    t.scale = glm::vec3{1.0f, 1.0f, 1.0f} + ((bLen/aLen - 1.0f)*aDir);
    t.position = bMid - aMid;

    return t;
}

osc::Transform osc::YToYCylinderToSegmentTransform(Segment const& s, float radius) noexcept
{
    Segment cylinderLine{{0.0f, -1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};
    Transform t = SegmentToSegmentTransform(cylinderLine, s);
    t.scale.x = radius;
    t.scale.z = radius;
    return t;
}

osc::Transform osc::YToYConeToSegmentTransform(Segment const& s, float radius) noexcept
{
    return YToYCylinderToSegmentTransform(s, radius);
}

glm::mat3 osc::ToMat3(Transform const& t) noexcept
{
    glm::mat3 rv = glm::toMat3(t.rotation);

    rv[0][0] *= t.scale.x;
    rv[0][1] *= t.scale.x;
    rv[0][2] *= t.scale.x;

    rv[1][0] *= t.scale.y;
    rv[1][1] *= t.scale.y;
    rv[1][2] *= t.scale.y;

    rv[2][0] *= t.scale.z;
    rv[2][1] *= t.scale.z;
    rv[2][2] *= t.scale.z;

    return rv;
}

glm::mat4 osc::ToMat4(Transform const& t) noexcept
{
    glm::mat4 rv = glm::toMat4(t.rotation);

    rv[0][0] *= t.scale.x;
    rv[0][1] *= t.scale.x;
    rv[0][2] *= t.scale.x;

    rv[1][0] *= t.scale.y;
    rv[1][1] *= t.scale.y;
    rv[1][2] *= t.scale.y;

    rv[2][0] *= t.scale.z;
    rv[2][1] *= t.scale.z;
    rv[2][2] *= t.scale.z;

    rv[3][0] = t.position.x;
    rv[3][1] = t.position.y;
    rv[3][2] = t.position.z;

    return rv;
}

glm::mat4 osc::ToInverseMat4(Transform const& t) noexcept
{
    glm::mat4 translater = glm::translate(glm::mat4{1.0f}, -t.position);
    glm::mat4 rotater = glm::toMat4(glm::conjugate(t.rotation));
    glm::mat4 scaler = glm::scale(glm::mat4{1.0f}, 1.0f/t.scale);

    return scaler * rotater * translater;
}

glm::mat3x3 osc::ToNormalMatrix(Transform const& t) noexcept
{
    return ToAdjugateMatrix(glm::transpose(ToMat3(t)));
}

glm::mat4 osc::ToNormalMatrix4(Transform const& t) noexcept
{
    return ToAdjugateMatrix(glm::transpose(ToMat3(t)));
}

osc::Transform osc::ToTransform(glm::mat4 const& mtx)
{
    Transform rv;
    glm::vec3 skew;
    glm::vec4 perspective;
    if (!glm::decompose(mtx, rv.scale, rv.rotation, rv.position, skew, perspective))
    {
        throw std::runtime_error{"failed to decompose a matrix into scale, rotation, etc."};
    }
    return rv;
}

glm::vec3 osc::TransformDirection(Transform const& t, glm::vec3 const& localDir) noexcept
{
    return glm::normalize(t.rotation * (t.scale * localDir));
}

glm::vec3 osc::InverseTransformDirection(Transform const& t, glm::vec3 const& dir) noexcept
{
    return glm::normalize((glm::conjugate(t.rotation) * dir) / t.scale);
}

glm::vec3 osc::TransformPoint(Transform const& t, glm::vec3 const& p) noexcept
{
    glm::vec3 rv = p;
    rv *= t.scale;
    rv = t.rotation * rv;
    rv += t.position;
    return rv;
}

glm::vec3 osc::InverseTransformPoint(Transform const& t, glm::vec3 const& p) noexcept
{
    glm::vec3 rv = p;
    rv -= t.position;
    rv = glm::conjugate(t.rotation) * rv;
    rv /= t.scale;
    return rv;
}

void osc::ApplyWorldspaceRotation(Transform& t,
    glm::vec3 const& eulerAngles,
    glm::vec3 const& rotationCenter) noexcept
{
    glm::quat q{eulerAngles};
    t.position = q*(t.position - rotationCenter) + rotationCenter;
    t.rotation = glm::normalize(q*t.rotation);
}

glm::vec3 osc::ExtractEulerAngleXYZ(Transform const& t) noexcept
{
    glm::vec3 rv;
    glm::extractEulerAngleXYZ(glm::toMat4(t.rotation), rv.x, rv.y, rv.z);
    return rv;
}

glm::vec3 osc::ExtractExtrinsicEulerAnglesXYZ(Transform const& t) noexcept
{
    return glm::eulerAngles(t.rotation);
}

bool osc::IsPointInRect(Rect const& r, glm::vec2 const& p) noexcept
{
    glm::vec2 relPos = p - r.p1;
    glm::vec2 dims = Dimensions(r);
    return (0.0f <= relPos.x && relPos.x <= dims.x) && (0.0f <= relPos.y && relPos.y <= dims.y);
}

std::optional<osc::RayCollision> osc::GetRayCollisionSphere(Line const& l, Sphere const& s) noexcept
{
    return GetRayCollisionSphereAnalytic(s, l);
}

std::optional<osc::RayCollision> osc::GetRayCollisionAABB(Line const& l, AABB const& bb) noexcept
{
    // intersect the ray with each axis-aligned slab for each dimension
    //
    // i.e. figure out where the line intersects the front+back of the AABB
    //      in (e.g.) X, then Y, then Z, and intersect those interactions such
    //      that if the intersection is ever empty (or, negative here) then there
    //      is no intersection
    float t0 = std::numeric_limits<float>::lowest();
    float t1 = std::numeric_limits<float>::max();
    for (glm::vec3::length_type i = 0; i < 3; ++i)
    {
        float invDir = 1.0f / l.dir[i];
        float tNear = (bb.min[i] - l.origin[i]) * invDir;
        float tFar = (bb.max[i] - l.origin[i]) * invDir;
        if (tNear > tFar)
        {
            std::swap(tNear, tFar);
        }
        t0 = std::max(t0, tNear);
        t1 = std::min(t1, tFar);

        if (t0 > t1)
        {
            return std::nullopt;
        }
    }

    return osc::RayCollision{t0, l.origin + t0*l.dir};
}

std::optional<osc::RayCollision> osc::GetRayCollisionPlane(Line const& l, Plane const& p) noexcept
{
    // see: https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-plane-and-ray-disk-intersection
    //
    // effectively, this is evaluating:
    //
    //     P, a point on the plane
    //     P0, the plane's origin (distance from world origin)
    //     N, the plane's normal
    //
    // against: dot(P-P0, N)
    //
    // which must equal zero for any point in the plane. Given that, a line can
    // be parameterized as `P = O + tD` where:
    //
    //     P, point along the line
    //     O, origin of line
    //     t, distance along line direction
    //     D, line direction
    //
    // sub the line equation into the plane equation, rearrange for `t` and you
    // can figure out how far a plane is along a line
    //
    // equation: t = dot(P0 - O, n) / dot(D, n)

    float denominator = glm::dot(p.normal, l.dir);

    if (std::abs(denominator) > 1e-6)
    {
        float numerator = glm::dot(p.origin - l.origin, p.normal);
        float distance = numerator / denominator;
        return osc::RayCollision{distance ,l.origin + distance*l.dir};
    }
    else
    {
        // the line is *very* parallel to the plane, which could cause
        // some divide-by-zero havok: pretend it didn't intersect
        return std::nullopt;
    }
}

std::optional<osc::RayCollision> osc::GetRayCollisionDisc(Line const& l, Disc const& d) noexcept
{
    // see: https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-plane-and-ray-disk-intersection

    // think of this as a ray-plane intersection test with the additional
    // constraint that the ray has to be within the radius of the disc

    Plane p;
    p.origin = d.origin;
    p.normal = d.normal;

    std::optional<RayCollision> maybePlaneCollision = GetRayCollisionPlane(l, p);

    if (!maybePlaneCollision)
    {
        return std::nullopt;
    }

    // figure out whether the plane hit is within the disc's radius
    glm::vec3 v = maybePlaneCollision->position - d.origin;
    float d2 = glm::dot(v, v);
    float r2 = glm::dot(d.radius, d.radius);

    if (d2 > r2)
    {
        return std::nullopt;
    }

    return maybePlaneCollision;
}

std::optional<osc::RayCollision> osc::GetRayCollisionTriangle(Line const& l, Triangle const& tri) noexcept
{
    // see: https://www.scratchapixel.com/lessons/3d-basic-rendering/ray-tracing-rendering-a-triangle/ray-triangle-intersection-geometric-solution

    // compute triangle normal
    glm::vec3 N = TriangleNormal(tri);

    // compute dot product between normal and ray
    float NdotR = glm::dot(N, l.dir);

    // if the dot product is small, then the ray is probably very parallel to
    // the triangle (or, perpendicular to the normal) and doesn't intersect
    if (std::abs(NdotR) < std::numeric_limits<float>::epsilon())
    {
        return std::nullopt;
    }

    // - v[0] is a known point on the plane
    // - N is a normal to the plane
    // - N.v[0] is the projection of v[0] onto N and indicates how long along N to go to hit some
    //   other point on the plane
    float D = glm::dot(N, tri.p0);

    // ok, that's one side of the equation
    //
    // - the other side of the equation is that the same is true for *any* point on the plane
    // - so: D = P.N also
    // - where P == O + tR (our line)
    // - expand: D = (O + tR).N
    // - rearrange:
    //
    //     D = O.N + t.R.N
    //     D - O.N = t.R.N
    //     (D - O.N)/(R.N) = t
    //
    // tah-dah: we have the ray distance
    float t = -(glm::dot(N, l.origin) - D) / NdotR;

    // if triangle plane is behind line then return early
    if (t < 0.0f)
    {
        return std::nullopt;
    }

    // intersection point on triangle plane, computed from line equation
    glm::vec3 P = l.origin + t*l.dir;

    // figure out if that point is inside the triangle's bounds using the
    // "inside-outside" test

    // test each triangle edge: {0, 1}, {1, 2}, {2, 0}
    for (size_t i = 0; i < 3; ++i)
    {
        glm::vec3 const& start = tri[i];
        glm::vec3 const& end = tri[(i+1)%3];

        // corner[n] to corner[n+1]
        glm::vec3 e = end - start;

        // corner[n] to P
        glm::vec3 c = P - start;

        // cross product of the above indicates whether the vectors are
        // clockwise or anti-clockwise with respect to eachover. It's a
        // right-handed coord system, so anti-clockwise produces a vector
        // that points in same direction as normal
        glm::vec3 ax = glm::cross(e, c);

        // if the dot product of that axis with the normal is <0.0f then
        // the point was "outside"
        if (glm::dot(ax, N) < 0.0f)
        {
            return std::nullopt;
        }
    }

    return osc::RayCollision{t, l.origin + t*l.dir};
}
