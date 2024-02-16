#include <oscar/Maths/AABB.h>
#include <oscar/Maths/Angle.h>
#include <oscar/Maths/BVH.h>
#include <oscar/Maths/CollisionTests.h>
#include <oscar/Maths/CommonFunctions.h>
#include <oscar/Maths/Disc.h>
#include <oscar/Maths/EasingFunctions.h>
#include <oscar/Maths/EulerPerspectiveCamera.h>
#include <oscar/Maths/FrameAxis.h>
#include <oscar/Maths/GeometricFunctions.h>
#include <oscar/Maths/Line.h>
#include <oscar/Maths/Mat.h>
#include <oscar/Maths/Mat3.h>
#include <oscar/Maths/Mat4.h>
#include <oscar/Maths/MathHelpers.h>
#include <oscar/Maths/MatrixFunctions.h>
#include <oscar/Maths/Plane.h>
#include <oscar/Maths/PolarPerspectiveCamera.h>
#include <oscar/Maths/Quat.h>
#include <oscar/Maths/QuaternionFunctions.h>
#include <oscar/Maths/RayCollision.h>
#include <oscar/Maths/Rect.h>
#include <oscar/Maths/Segment.h>
#include <oscar/Maths/Sphere.h>
#include <oscar/Maths/Tetrahedron.h>
#include <oscar/Maths/Transform.h>
#include <oscar/Maths/Triangle.h>
#include <oscar/Maths/TrigonometricFunctions.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/Maths/Vec4.h>
#include <oscar/Platform/Log.h>
#include <oscar/Utils/Assertions.h>
#include <oscar/Utils/At.h>
#include <oscar/Utils/EnumHelpers.h>

#include <cmath>
#include <algorithm>
#include <array>
#include <concepts>
#include <cstddef>
#include <functional>
#include <iostream>
#include <iterator>
#include <limits>
#include <memory>
#include <numbers>
#include <numeric>
#include <optional>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <stack>
#include <stdexcept>
#include <utility>

using namespace osc::literals;
using namespace osc;

// `AABB` implementation

std::ostream& osc::operator<<(std::ostream& o, AABB const& aabb)
{
    return o << "AABB(min = " << aabb.min << ", max = " << aabb.max << ')';
}


// BVH implementation

// BVH helpers
namespace
{
    AABB Union(std::span<BVHPrim const> prims)
    {
        AABB rv = prims.front().getBounds();
        for (auto it = prims.begin() + 1; it != prims.end(); ++it)
        {
            rv = Union(rv, it->getBounds());
        }
        return rv;
    }

    bool HasAVolume(Triangle const& t)
    {
        return !(t.p0 == t.p1 || t.p0 == t.p2 || t.p1 == t.p2);
    }

    // recursively build the BVH
    void BVH_RecursiveBuild(
        std::vector<BVHNode>& nodes,
        std::vector<BVHPrim>& prims,
        ptrdiff_t const begin,
        ptrdiff_t const n)
    {
        if (n == 1)
        {
            // recursion bottomed out: create a leaf node
            nodes.push_back(BVHNode::leaf(
                prims.at(begin).getBounds(),
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
            AABB const aabb = Union({prims.begin() + begin, static_cast<size_t>(n)});

            // compute slicing position along the longest dimension
            auto const longestDimIdx = LongestDimIndex(aabb);
            float const midpointX2 = aabb.min[longestDimIdx] + aabb.max[longestDimIdx];

            // returns true if a given primitive is below the midpoint along the dim
            auto const isBelowMidpoint = [longestDimIdx, midpointX2](BVHPrim const& p)
            {
                float const primMidpointX2 = p.getBounds().min[longestDimIdx] + p.getBounds().max[longestDimIdx];
                return primMidpointX2 <= midpointX2;
            };

            // partition prims into above/below the midpoint
            ptrdiff_t const end = begin + n;
            auto const it = std::partition(prims.begin() + begin, prims.begin() + end, isBelowMidpoint);

            midpoint = std::distance(prims.begin(), it);
            if (midpoint == begin || midpoint == end)
            {
                // edge-case: failed to spacially partition: just naievely partition
                midpoint = begin + n/2;
            }

            internalNodeLoc = static_cast<ptrdiff_t>(nodes.size());

            // push the internal node
            nodes.push_back(BVHNode::node(
                aabb,
                0  // the number of left-hand nodes is set later
            ));
        }

        // build left-hand subtree
        BVH_RecursiveBuild(nodes, prims, begin, midpoint-begin);

        // the left-hand build allocated nodes for the left hand side contiguously in memory
        ptrdiff_t numLhsNodes = (static_cast<ptrdiff_t>(nodes.size()) - 1) - internalNodeLoc;
        OSC_ASSERT(numLhsNodes > 0);
        nodes[internalNodeLoc].setNumLhsNodes(numLhsNodes);

        // build right node
        BVH_RecursiveBuild(nodes, prims, midpoint, (begin + n) - midpoint);
        OSC_ASSERT(internalNodeLoc+numLhsNodes < static_cast<ptrdiff_t>(nodes.size()));
    }

    // returns true if something hit (recursively)
    //
    // populates outparam with all AABB hits in depth-first order
    bool BVH_ForEachRayAABBCollisionsRecursive(
        std::span<BVHNode const> nodes,
        std::span<BVHPrim const> prims,
        Line const& ray,
        ptrdiff_t nodeidx,
        std::function<void(BVHCollision)> const& callback)
    {
        BVHNode const& node = nodes[nodeidx];

        // check ray-AABB intersection with the BVH node
        std::optional<RayCollision> res = GetRayCollisionAABB(ray, node.getBounds());

        if (!res)
        {
            return false;  // no intersection with this node at all
        }

        if (node.isLeaf())
        {
            // it's a leaf node, so we've sucessfully found the AABB that intersected
            callback(BVHCollision{
                res->distance,
                res->position,
                prims[node.getFirstPrimOffset()].getID(),
            });
            return true;
        }

        // else: we've "hit" an internal node and need to recurse to find the leaf
        bool const lhs = BVH_ForEachRayAABBCollisionsRecursive(nodes, prims, ray, nodeidx+1, callback);
        bool const rhs = BVH_ForEachRayAABBCollisionsRecursive(nodes, prims, ray, nodeidx+node.getNumLhsNodes()+1, callback);
        return lhs || rhs;
    }

    template<std::unsigned_integral TIndex>
    std::optional<BVHCollision> BVH_GetClosestRayIndexedTriangleCollisionRecursive(
        std::span<BVHNode const> nodes,
        std::span<BVHPrim const> prims,
        std::span<Vec3 const> verts,
        std::span<TIndex const> indices,
        Line const& ray,
        float& closest,
        ptrdiff_t nodeidx)
    {
        BVHNode const& node = nodes[nodeidx];
        std::optional<RayCollision> const res = GetRayCollisionAABB(ray, node.getBounds());

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

            BVHPrim const& p = At(prims, node.getFirstPrimOffset());

            Triangle const triangle =
            {
                At(verts, At(indices, p.getID())),
                At(verts, At(indices, p.getID()+1)),
                At(verts, At(indices, p.getID()+2)),
            };

            std::optional<RayCollision> const rayTriangleColl = GetRayCollisionTriangle(ray, triangle);

            if (rayTriangleColl && rayTriangleColl->distance < closest)
            {
                closest = rayTriangleColl->distance;
                return BVHCollision{rayTriangleColl->distance, rayTriangleColl->position, p.getID()};
            }
            else
            {
                return std::nullopt;  // it didn't collide with the triangle
            }
        }

        // else: internal node: recurse
        std::optional<BVHCollision> const lhs = BVH_GetClosestRayIndexedTriangleCollisionRecursive(
            nodes,
            prims,
            verts,
            indices,
            ray,
            closest,
            nodeidx+1
        );
        std::optional<BVHCollision> const rhs = BVH_GetClosestRayIndexedTriangleCollisionRecursive(
            nodes,
            prims,
            verts,
            indices,
            ray,
            closest,
            nodeidx+node.getNumLhsNodes()+1
        );
        return rhs ? rhs : lhs;
    }

    template<std::unsigned_integral TIndex>
    void BuildFromIndexedTriangles(
        std::vector<BVHNode>& nodes,
        std::vector<BVHPrim>& prims,
        std::span<Vec3 const> verts,
        std::span<TIndex const> indices)
    {
        // clear out any old data
        nodes.clear();
        prims.clear();

        // build up the prim list for each triangle
        prims.reserve(indices.size()/3);  // good guess
        for (size_t i = 0; (i+2) < indices.size(); i += 3)
        {
            Triangle const t
            {
                At(verts, indices[i]),
                At(verts, indices[i+1]),
                At(verts, indices[i+2]),
            };

            if (HasAVolume(t))
            {
                prims.emplace_back(static_cast<ptrdiff_t>(i), AABBFromTriangle(t));
            }
        }

        if (!prims.empty())
        {
            BVH_RecursiveBuild(nodes, prims, 0, static_cast<ptrdiff_t>(prims.size()));
        }
    }

    template<std::unsigned_integral TIndex>
    std::optional<BVHCollision> GetClosestRayIndexedTriangleCollision(
        std::span<BVHNode const> nodes,
        std::span<BVHPrim const> prims,
        std::span<Vec3 const> verts,
        std::span<TIndex const> indices,
        Line const& ray)
    {
        if (nodes.empty() || prims.empty() || indices.empty())
        {
            return std::nullopt;
        }

        float closest = std::numeric_limits<float>::max();
        return BVH_GetClosestRayIndexedTriangleCollisionRecursive(nodes, prims, verts, indices, ray, closest, 0);
    }
}

void osc::BVH::clear()
{
    m_Nodes.clear();
    m_Prims.clear();
}

void osc::BVH::buildFromIndexedTriangles(std::span<Vec3 const> verts, std::span<uint16_t const> indices)
{
    BuildFromIndexedTriangles<uint16_t>(
        m_Nodes,
        m_Prims,
        verts,
        indices
    );
}

void osc::BVH::buildFromIndexedTriangles(std::span<Vec3 const> verts, std::span<uint32_t const> indices)
{
    BuildFromIndexedTriangles<uint32_t>(
        m_Nodes,
        m_Prims,
        verts,
        indices
    );
}

std::optional<BVHCollision> osc::BVH::getClosestRayIndexedTriangleCollision(
    std::span<Vec3 const> verts,
    std::span<uint16_t const> indices,
    Line const& line) const
{
    return GetClosestRayIndexedTriangleCollision<uint16_t>(
        m_Nodes,
        m_Prims,
        verts,
        indices,
        line
    );
}

std::optional<BVHCollision> osc::BVH::getClosestRayIndexedTriangleCollision(
    std::span<Vec3 const> verts,
    std::span<uint32_t const> indices,
    Line const& line) const
{
    return GetClosestRayIndexedTriangleCollision<uint32_t>(
        m_Nodes,
        m_Prims,
        verts,
        indices,
        line
    );
}

void osc::BVH::buildFromAABBs(std::span<AABB const> aabbs)
{
    // clear out any old data
    clear();

    // build up prim list for each AABB (just copy the AABB)
    m_Prims.reserve(aabbs.size());  // good guess
    for (ptrdiff_t i = 0; i < std::ssize(aabbs); ++i)
    {
        if (!IsAPoint(aabbs[i]))
        {
            m_Prims.emplace_back(static_cast<ptrdiff_t>(i), aabbs[i]);
        }
    }

    if (!m_Prims.empty())
    {
        BVH_RecursiveBuild(
            m_Nodes,
            m_Prims,
            0,
            std::ssize(m_Prims)
        );
    }
}

void osc::BVH::forEachRayAABBCollision(
    Line const& ray,
    std::function<void(BVHCollision)> const& callback) const
{
    if (m_Nodes.empty() || m_Prims.empty())
    {
        return;
    }

    BVH_ForEachRayAABBCollisionsRecursive(
        m_Nodes,
        m_Prims,
        ray,
        0,
        callback
    );
}

bool osc::BVH::empty() const
{
    return m_Nodes.empty();
}

size_t osc::BVH::getMaxDepth() const
{
    size_t cur = 0;
    size_t maxdepth = 0;
    std::stack<size_t> stack;

    while (cur < m_Nodes.size())
    {
        if (m_Nodes[cur].isLeaf())
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
                cur = next + m_Nodes[next].getNumLhsNodes() + 1;
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

std::optional<AABB> osc::BVH::getRootAABB() const
{
    return !m_Nodes.empty() ? m_Nodes.front().getBounds() : std::optional<AABB>{};
}

void osc::BVH::forEachLeafNode(std::function<void(BVHNode const&)> const& f) const
{
    for (BVHNode const& node : m_Nodes)
    {
        if (node.isLeaf())
        {
            f(node);
        }
    }
}

void osc::BVH::forEachLeafOrInnerNodeUnordered(std::function<void(BVHNode const&)> const& f) const
{
    for (BVHNode const& node : m_Nodes)
    {
        f(node);
    }
}

// `Disc` implementation

std::ostream& osc::operator<<(std::ostream& o, Disc const& d)
{
    return o << "Disc(origin = " << d.origin << ", normal = " << d.normal << ", radius = " << d.radius << ')';
}


// `EulerPerspectiveCamera` implementation

osc::EulerPerspectiveCamera::EulerPerspectiveCamera() :
    origin{},
    pitch{},
    yaw{-180_deg},
    verticalFOV{35_deg},
    znear{0.1f},
    zfar{1000.0f}
{
}

Vec3 osc::EulerPerspectiveCamera::getFront() const
{
    return normalize(Vec3
    {
        cos(yaw) * cos(pitch),
        sin(pitch),
        sin(yaw) * cos(pitch),
    });
}

Vec3 osc::EulerPerspectiveCamera::getUp() const
{
    return Vec3{0.0f, 1.0f, 0.0f};
}

Vec3 osc::EulerPerspectiveCamera::getRight() const
{
    return normalize(cross(getFront(), getUp()));
}

Mat4 osc::EulerPerspectiveCamera::getViewMtx() const
{
    return look_at(origin, origin + getFront(), getUp());
}

Mat4 osc::EulerPerspectiveCamera::getProjMtx(float aspectRatio) const
{
    return Perspective(verticalFOV, aspectRatio, znear, zfar);
}


// `Line` implementation

std::ostream& osc::operator<<(std::ostream& o, Line const& l)
{
    return o << "Line(origin = " << l.origin << ", direction = " << l.direction << ')';
}


// `Plane` implementation

std::ostream& osc::operator<<(std::ostream& o, Plane const& p)
{
    return o << "Plane(origin = " << p.origin << ", normal = " << p.normal << ')';
}


// `PolarPerspectiveCamera` implementation

namespace
{
    Vec3 PolarToCartesian(Vec3 focus, float radius, Radians theta, Radians phi)
    {
        float x = radius * sin(theta) * cos(phi);
        float y = radius * sin(phi);
        float z = radius * cos(theta) * cos(phi);

        return -focus + Vec3{x, y, z};
    }
}

osc::PolarPerspectiveCamera::PolarPerspectiveCamera() :
    radius{1.0f},
    theta{45_deg},
    phi{45_deg},
    focusPoint{0.0f, 0.0f, 0.0f},
    verticalFOV{35_deg},
    znear{0.1f},
    zfar{100.0f}
{
}

void osc::PolarPerspectiveCamera::reset()
{
    *this = {};
}

void osc::PolarPerspectiveCamera::pan(float aspectRatio, Vec2 delta)
{
    auto horizontalFOV = VerticalToHorizontalFOV(verticalFOV, aspectRatio);

    // how much panning is done depends on how far the camera is from the
    // origin (easy, with polar coordinates) *and* the FoV of the camera.
    float xAmt = delta.x * (2.0f * tan(horizontalFOV / 2.0f) * radius);
    float yAmt = -delta.y * (2.0f * tan(verticalFOV / 2.0f) * radius);

    // this assumes the scene is not rotated, so we need to rotate these
    // axes to match the scene's rotation
    Vec4 defaultPanningAx = {xAmt, yAmt, 0.0f, 1.0f};
    auto rotTheta = Rotate(Identity<Mat4>(), theta, Vec3{0.0f, 1.0f, 0.0f});
    auto thetaVec = normalize(Vec3{sin(theta), 0.0f, cos(theta)});
    auto phiAxis = cross(thetaVec, Vec3{0.0, 1.0f, 0.0f});
    auto rotPhi = Rotate(Identity<Mat4>(), phi, phiAxis);

    Vec4 panningAxes = rotPhi * rotTheta * defaultPanningAx;
    focusPoint += Vec3{panningAxes};
}

void osc::PolarPerspectiveCamera::drag(Vec2 delta)
{
    theta += 360_deg * -delta.x;
    phi += 360_deg * delta.y;
}

void osc::PolarPerspectiveCamera::rescaleZNearAndZFarBasedOnRadius()
{
    // znear and zfar are only really dicated by the camera's radius, because
    // the radius is effectively the distance from the camera's focal point

    znear = 0.02f * radius;
    zfar = 20.0f * radius;
}

Mat4 osc::PolarPerspectiveCamera::getViewMtx() const
{
    // camera: at a fixed position pointing at a fixed origin. The "camera"
    // works by translating + rotating all objects around that origin. Rotation
    // is expressed as polar coordinates. Camera panning is represented as a
    // translation vector.

    // this maths is a complete shitshow and I apologize. It just happens to work for now. It's a polar coordinate
    // system that shifts the world based on the camera pan

    auto rotTheta = Rotate(Identity<Mat4>(), -theta, Vec3{0.0f, 1.0f, 0.0f});
    auto thetaVec = normalize(Vec3{sin(theta), 0.0f, cos(theta)});
    auto phiAxis = cross(thetaVec, Vec3{0.0, 1.0f, 0.0f});
    auto rotPhi = Rotate(Identity<Mat4>(), -phi, phiAxis);
    auto panTranslate = Translate(Identity<Mat4>(), focusPoint);
    return look_at(
        Vec3(0.0f, 0.0f, radius),
        Vec3(0.0f, 0.0f, 0.0f),
        Vec3{0.0f, 1.0f, 0.0f}) * rotTheta * rotPhi * panTranslate;
}

Mat4 osc::PolarPerspectiveCamera::getProjMtx(float aspectRatio) const
{
    return Perspective(verticalFOV, aspectRatio, znear, zfar);
}

Vec3 osc::PolarPerspectiveCamera::getPos() const
{
    return PolarToCartesian(focusPoint, radius, theta, phi);
}

Vec2 osc::PolarPerspectiveCamera::projectOntoScreenRect(Vec3 const& worldspaceLoc, Rect const& screenRect) const
{
    Vec2 dims = Dimensions(screenRect);
    Mat4 MV = getProjMtx(dims.x/dims.y) * getViewMtx();

    Vec4 ndc = MV * Vec4{worldspaceLoc, 1.0f};
    ndc /= ndc.w;  // perspective divide

    Vec2 ndc2D;
    ndc2D = {ndc.x, -ndc.y};        // [-1, 1], Y points down
    ndc2D += 1.0f;                  // [0, 2]
    ndc2D *= 0.5f;                  // [0, 1]
    ndc2D *= dims;                  // [0, w]
    ndc2D += screenRect.p1;         // [x, x + w]

    return ndc2D;
}

Line osc::PolarPerspectiveCamera::unprojectTopLeftPosToWorldRay(Vec2 pos, Vec2 dims) const
{
    Mat4 const viewMatrix = getViewMtx();
    Mat4 const projectionMatrix = getProjMtx(dims.x/dims.y);

    return PerspectiveUnprojectTopLeftScreenPosToWorldRay(
        pos / dims,
        this->getPos(),
        viewMatrix,
        projectionMatrix
    );
}

PolarPerspectiveCamera osc::CreateCameraWithRadius(float r)
{
    PolarPerspectiveCamera rv;
    rv.radius = r;
    return rv;
}

PolarPerspectiveCamera osc::CreateCameraFocusedOn(AABB const& aabb)
{
    PolarPerspectiveCamera rv;
    AutoFocus(rv, aabb);
    return rv;
}

Vec3 osc::RecommendedLightDirection(PolarPerspectiveCamera const& c)
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
    Radians const theta = c.theta + 22.5_deg;

    // #549: phi shouldn't track with the camera, because changing the "height"/"slope"
    // of the camera with shadow rendering (#10) looks bizzare
    Radians const phi = 45_deg;

    Vec3 const p = PolarToCartesian(c.focusPoint, c.radius, theta, phi);

    return normalize(-c.focusPoint - p);
}

void osc::FocusAlongX(PolarPerspectiveCamera& camera)
{
    camera.theta = 90_deg;
    camera.phi = 0_deg;
}

void osc::FocusAlongMinusX(PolarPerspectiveCamera& camera)
{
    camera.theta = -90_deg;
    camera.phi = 0_deg;
}

void osc::FocusAlongY(PolarPerspectiveCamera& camera)
{
    camera.theta = 0_deg;
    camera.phi = 90_deg;
}

void osc::FocusAlongMinusY(PolarPerspectiveCamera& camera)
{
    camera.theta = 0_deg;
    camera.phi = -90_deg;
}

void osc::FocusAlongZ(PolarPerspectiveCamera& camera)
{
    camera.theta = 0_deg;
    camera.phi = 0_deg;
}

void osc::FocusAlongMinusZ(PolarPerspectiveCamera& camera)
{
    camera.theta = 180_deg;
    camera.phi = 0_deg;
}

void osc::ZoomIn(PolarPerspectiveCamera& camera)
{
    camera.radius *= 0.8f;
}

void osc::ZoomOut(PolarPerspectiveCamera& camera)
{
    camera.radius *= 1.2f;
}

void osc::Reset(PolarPerspectiveCamera& camera)
{
    camera = {};
    camera.theta = 45_deg;
    camera.phi = 45_deg;
}

void osc::AutoFocus(PolarPerspectiveCamera& camera, AABB const& elementAABB, float aspectRatio)
{
    Sphere const s = ToSphere(elementAABB);
    Radians const smallestFOV = aspectRatio > 1.0f ? camera.verticalFOV : VerticalToHorizontalFOV(camera.verticalFOV, aspectRatio);

    // auto-focus the camera with a minimum radius of 1m
    //
    // this will break autofocusing on very small models (e.g. insect legs) but
    // handles the edge-case of autofocusing an empty model (#552), which is a
    // more common use-case (e.g. for new users and users making human-sized models)
    camera.focusPoint = -s.origin;
    camera.radius = std::max(s.radius / tan(smallestFOV/2.0), 1.0f);
    camera.rescaleZNearAndZFarBasedOnRadius();
}


// `Rect` implementation

std::ostream& osc::operator<<(std::ostream& o, Rect const& r)
{
    return o << "Rect(p1 = " << r.p1 << ", p2 = " << r.p2 << ")";
}


// `Segment` implementation

std::ostream& osc::operator<<(std::ostream& o, Segment const& d)
{
    return o << "Segment(p1 = " << d.p1 << ", p2 = " << d.p2 << ')';
}


// `Sphere` implementation

std::ostream& osc::operator<<(std::ostream& o, Sphere const& s)
{
    return o << "Sphere(origin = " << s.origin << ", radius = " << s.radius << ')';
}


// `Tetrahedron` implementatioon

// returns the volume of a given tetrahedron, defined as 4 points in space
float osc::Volume(Tetrahedron const& t)
{
    // sources:
    //
    // http://forums.cgsociety.org/t/how-to-calculate-center-of-mass-for-triangular-mesh/1309966
    // https://stackoverflow.com/questions/9866452/calculate-volume-of-any-tetrahedron-given-4-points

    Mat4 const m
    {
        Vec4{t[0], 1.0f},
        Vec4{t[1], 1.0f},
        Vec4{t[2], 1.0f},
        Vec4{t[3], 1.0f},
    };

    return determinant(m) / 6.0f;
}

// returns spatial centerpoint of a given tetrahedron
Vec3 osc::Center(Tetrahedron const& t)
{
    // arithmetic mean of tetrahedron vertices
    return std::reduce(t.begin(), t.end()) / static_cast<float>(t.size());
}


// `Transform` implementation

std::ostream& osc::operator<<(std::ostream& o, Transform const& t)
{
    return o << "Transform(position = " << t.position << ", rotation = " << t.rotation << ", scale = " << t.scale << ')';
}

Vec3 osc::operator*(Transform const& lhs, Vec3 const& rhs)
{
    return TransformPoint(lhs, rhs);
}

Transform& osc::operator+=(Transform& lhs, Transform const& rhs)
{
    lhs.position += rhs.position;
    lhs.rotation += rhs.rotation;
    lhs.scale += rhs.scale;
    return lhs;
}

Transform& osc::operator/=(Transform& lhs, float rhs)
{
    lhs.position /= rhs;
    lhs.rotation /= rhs;
    lhs.scale /= rhs;
    return lhs;
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
        QuadraticFormulaResult res{};

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

    std::optional<RayCollision> GetRayCollisionSphereAnalytic(Sphere const& s, Line const& l)
    {
        // see: https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-sphere-intersection

        Vec3 L = l.origin - s.origin;

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

        float a = dot(l.direction, l.direction);  // always == 1.0f if d is normalized
        float b = 2.0f * dot(l.direction, L);
        float c = dot(L, L) - dot(s.radius, s.radius);

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

        return RayCollision{t0, l.origin + t0*l.direction};
    }

    template<std::floating_point TReal>
    bool IsEqualWithinRelativeError(TReal a , TReal b, TReal relativeError)
    {
        // inspired from:
        //
        // - https://stackoverflow.com/questions/17333/what-is-the-most-effective-way-for-float-and-double-comparison
        //
        // but, specifically, you should read the section `Epsilon comparisons` here:
        //
        // - https://randomascii.wordpress.com/2012/02/25/comparing-floating-point-numbers-2012-edition/

        auto const difference = std::abs(a - b);
        auto const permittedAbsoluteError = relativeError * std::max(std::abs(a), std::abs(b));
        return difference <= permittedAbsoluteError;
    }
}


// MathHelpers

Quat osc::Rotation(Vec3 const& src, Vec3 const& dest)
{
    return rotation(src, dest);
}

Quat osc::AngleAxis(Radians angle, Vec3 const& axis)
{
    return angle_axis(angle, axis);
}

Mat4 osc::LookAt(Vec3 const& eye, Vec3 const& center, Vec3 const& up)
{
    return look_at(eye, center, up);
}

Radians osc::VerticalToHorizontalFOV(Radians verticalFOV, float aspectRatio)
{
    // https://en.wikipedia.org/wiki/Field_of_view_in_video_games#Field_of_view_calculations

    return 2.0f * atan(tan(verticalFOV / 2.0f) * aspectRatio);
}

Mat4 osc::Perspective(Radians verticalFOV, float aspectRatio, float zNear, float zFar)
{
    if (std::fabs(aspectRatio - std::numeric_limits<float>::epsilon()) > 0.0f) {
        return perspective(verticalFOV, aspectRatio, zNear, zFar);
    }
    // edge-case: some UIs ask for a perspective matrix on first frame before
    // aspect ratio is known or the aspect ratio is NaN because of a division
    // by zero
    return Mat4{1.0f};
}

Mat4 osc::Ortho(float left, float right, float bottom, float top, float zNear, float zFar)
{
    return ortho(left, right, bottom, top, zNear, zFar);
}

Mat4 osc::Inverse(Mat4 const& mat)
{
    return inverse(mat);
}

Quat osc::Inverse(Quat const& q)
{
    return inverse(q);
}

Mat4 osc::Scale(Mat4 const& m, Vec3 const& v)
{
    return scale(m, v);
}

Mat4 osc::Rotate(Mat4 const& m, Radians angle, Vec3 const& axis)
{
    return rotate(m, angle, axis);
}

Mat4 osc::Translate(Mat4 const& m, Vec3 const& v)
{
    return translate(m, v);
}

Quat osc::QuatCast(Mat3 const& m)
{
    return quat_cast(Mat3{m});
}

Mat3 osc::ToMat3(Quat const& q)
{
    return mat3_cast(q);
}

Eulers osc::EulerAngles(Quat const& q)
{
    return euler_angles(normalize(q));
}

// returns `true` if the values of `a` and `b` are effectively equal
//
// this algorithm is designed to be correct, rather than fast
bool osc::IsEffectivelyEqual(double a, double b)
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

bool osc::IsLessThanOrEffectivelyEqual(double a, double b)
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

bool osc::IsEqualWithinRelativeError(double a , double b, double relativeError)
{
    return ::IsEqualWithinRelativeError<double>(a, b, relativeError);
}

bool osc::IsEqualWithinRelativeError(float a, float b, float relativeError)
{
    return ::IsEqualWithinRelativeError<float>(a, b, relativeError);
}

bool osc::IsEqualWithinAbsoluteError(float a, float b, float absError)
{
    auto const difference = std::abs(a - b);
    return difference <= absError;
}

Vec3::size_type osc::LongestDimIndex(Vec3 const& v)
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

Vec2::size_type osc::LongestDimIndex(Vec2 v)
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

Vec2i::size_type osc::LongestDimIndex(Vec2i v)
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

float osc::LongestDim(Vec3 const& v)
{
    return v[LongestDimIndex(v)];
}

float osc::LongestDim(Vec2 v)
{
    return v[LongestDimIndex(v)];
}

Vec2i::value_type osc::LongestDim(Vec2i v)
{
    return v[LongestDimIndex(v)];
}

float osc::AspectRatio(Vec2i v)
{
    return static_cast<float>(v.x) / static_cast<float>(v.y);
}

float osc::AspectRatio(Vec2 v)
{
    return v.x/v.y;
}

Vec2 osc::Midpoint(Vec2 a, Vec2 b)
{
    return 0.5f*(a+b);
}

Vec3 osc::Midpoint(Vec3 const& a, Vec3 const& b)
{
    return 0.5f*(a+b);
}

Vec3 osc::Midpoint(std::span<Vec3 const> vs)
{
    return std::reduce(vs.begin(), vs.end()) / static_cast<float>(vs.size());
}


// Geometry

Vec3 osc::KahanSum(std::span<Vec3 const> vs)
{
    Vec3 sum{};  // accumulator
    Vec3 c{};    // running compensation of low-order bits

    for (Vec3 const& v : vs)
    {
        Vec3 const y = v - c;    // subtract the compensation amount from the next number
        Vec3 const t = sum + y;  // perform the summation (might lose information)

        c = (t - sum) - y;            // (t-sum) yields the retained (high-order) parts of `y`, so `c` contains the "lost" information
        sum = t;                      // CAREFUL: algebreically, `c` always == 0 - despite the computer's (actual) limited precision, the compiler might elilde all of this
    }

    return sum;
}

Vec3 osc::NumericallyStableAverage(std::span<Vec3 const> vs)
{
    Vec3 const sum = KahanSum(vs);
    return sum / static_cast<float>(vs.size());
}

Vec3 osc::TriangleNormal(Triangle const& tri)
{
    Vec3 const ab = tri.p1 - tri.p0;
    Vec3 const ac = tri.p2 - tri.p0;
    Vec3 const perpendiular = cross(ab, ac);
    return normalize(perpendiular);
}

Mat3 osc::ToAdjugateMatrix(Mat3 const& m)
{
    // google: "Adjugate Matrix": it's related to the cofactor matrix and is
    // related to the inverse of a matrix through:
    //
    //     inverse(M) = Adjugate(M) / determinant(M);

    Mat3 rv;
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

Mat3 osc::ToNormalMatrix(Mat4 const& m)
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

    Mat3 const topLeft{m};
    return ToAdjugateMatrix(transpose(topLeft));
}

Mat4 osc::ToNormalMatrix4(Mat4 const& m)
{
    return ToNormalMatrix(m);
}

Mat4 osc::Dir1ToDir2Xform(Vec3 const& dir1, Vec3 const& dir2)
{
    // this is effectively a rewrite of glm::rotation(vec3 const&, vec3 const& dest);

    float const cosTheta = dot(dir1, dir2);

    if(cosTheta >= static_cast<float>(1.0f) - std::numeric_limits<float>::epsilon())
    {
        // `a` and `b` point in the same direction: return identity transform
        return Identity<Mat4>();
    }

    Radians theta{};
    Vec3 rotationAxis{};
    if(cosTheta < static_cast<float>(-1.0f) + std::numeric_limits<float>::epsilon())
    {
        // `a` and `b` point in opposite directions
        //
        // - there is no "ideal" rotation axis
        // - so we try "guessing" one and hope it's good (then try another if it isn't)

        rotationAxis = cross(Vec3{0.0f, 0.0f, 1.0f}, dir1);
        if (length2(rotationAxis) < std::numeric_limits<float>::epsilon())
        {
            // bad luck: they were parallel - use a different axis
            rotationAxis = cross(Vec3{1.0f, 0.0f, 0.0f}, dir1);
        }

        theta = 180_deg;
        rotationAxis = normalize(rotationAxis);
    }
    else
    {
        theta = acos(cosTheta);
        rotationAxis = normalize(cross(dir1, dir2));
    }

    return Rotate(Identity<Mat4>(), theta, rotationAxis);
}

Eulers osc::ExtractEulerAngleXYZ(Quat const& q)
{
    return extract_eulers_xyz(mat4_cast(q));
}

Eulers osc::ExtractEulerAngleXYZ(Mat4 const& m)
{
    return extract_eulers_xyz(m);
}

Vec2 osc::TopleftRelPosToNDCPoint(Vec2 relpos)
{
    relpos.y = 1.0f - relpos.y;
    return 2.0f*relpos - 1.0f;
}

Vec2 osc::NDCPointToTopLeftRelPos(Vec2 ndcPos)
{
    ndcPos = (ndcPos + 1.0f) * 0.5f;
    ndcPos.y = 1.0f - ndcPos.y;
    return ndcPos;
}

Vec4 osc::TopleftRelPosToNDCCube(Vec2 relpos)
{
    return {TopleftRelPosToNDCPoint(relpos), -1.0f, 1.0f};
}

Line osc::PerspectiveUnprojectTopLeftScreenPosToWorldRay(
    Vec2 relpos,
    Vec3 cameraWorldspaceOrigin,
    Mat4 const& viewMatrix,
    Mat4 const& projMatrix)
{
    // position of point, as if it were on the front of the 3D NDC cube
    Vec4 lineOriginNDC = TopleftRelPosToNDCCube(relpos);

    Vec4 lineOriginView = inverse(projMatrix) * lineOriginNDC;
    lineOriginView /= lineOriginView.w;  // perspective divide

    // location of mouse in worldspace
    Vec3 lineOriginWorld = Vec3{inverse(viewMatrix) * lineOriginView};

    // direction vector from camera to mouse location (i.e. the projection)
    Vec3 lineDirWorld = normalize(lineOriginWorld - cameraWorldspaceOrigin);

    Line rv{};
    rv.direction = lineDirWorld;
    rv.origin = lineOriginWorld;
    return rv;
}

Vec2 osc::MinValuePerDimension(Rect const& r)
{
    return elementwise_min(r.p1, r.p2);
}

float osc::Area(Rect const& r)
{
    auto d = Dimensions(r);
    return d.x * d.y;
}

Vec2 osc::Dimensions(Rect const& r)
{
    return abs(r.p2 - r.p1);
}

Vec2 osc::BottomLeft(Rect const& r)
{
    return Vec2{min(r.p1.x, r.p2.x), max(r.p1.y, r.p2.y)};
}

float osc::AspectRatio(Rect const& r)
{
    Vec2 dims = Dimensions(r);
    return dims.x/dims.y;
}

Vec2 osc::Midpoint(Rect const& r)
{
    return 0.5f * (r.p1 + r.p2);
}

Rect osc::BoundingRectOf(std::span<Vec2 const> vs)
{
    if (vs.empty())
    {
        return Rect{};  // edge-case
    }

    Rect rv{vs.front(), vs.front()};
    for (auto it = vs.begin()+1; it != vs.end(); ++it)
    {
        rv.p1 = elementwise_min(rv.p1, *it);
        rv.p2 = elementwise_max(rv.p2, *it);
    }
    return rv;
}

Rect osc::Expand(Rect const& rect, float amt)
{
    Rect rv
    {
        elementwise_min(rect.p1, rect.p2),
        elementwise_max(rect.p1, rect.p2)
    };
    rv.p1.x -= amt;
    rv.p2.x += amt;
    rv.p1.y -= amt;
    rv.p2.y += amt;
    return rv;
}

Rect osc::Expand(Rect const& rect, Vec2 amt)
{
    Rect rv
    {
        elementwise_min(rect.p1, rect.p2),
        elementwise_max(rect.p1, rect.p2)
    };
    rv.p1.x -= amt.x;
    rv.p2.x += amt.x;
    rv.p1.y -= amt.y;
    rv.p2.y += amt.y;
    return rv;
}

Rect osc::Clamp(Rect const& r, Vec2 const& min, Vec2 const& max)
{
    return
    {
        clamp(r.p1, min, max),
        clamp(r.p2, min, max),
    };
}

Rect osc::NdcRectToScreenspaceViewportRect(Rect const& ndcRect, Rect const& viewport)
{
    Vec2 const viewportDims = Dimensions(viewport);

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

Sphere osc::BoundingSphereOf(std::span<Vec3 const> points)
{
    // edge-case: no points provided
    if (points.empty())
    {
        return Sphere{.radius = 0.0f};
    }

    Vec3 const origin = Midpoint(AABBFromVerts(points));

    float r2 = 0.0f;
    for (Vec3 const& pos : points)
    {
        r2 = max(r2, length2(pos - origin));
    }

    return {.origin = origin, .radius = std::sqrt(r2)};
}

Sphere osc::ToSphere(AABB const& aabb)
{
    return BoundingSphereOf(ToCubeVerts(aabb));
}

Mat4 osc::FromUnitSphereMat4(Sphere const& s)
{
    return translate(Identity<Mat4>(), s.origin) * scale(Identity<Mat4>(), {s.radius, s.radius, s.radius});
}

Mat4 osc::SphereToSphereMat4(Sphere const& a, Sphere const& b)
{
    float s = b.radius/a.radius;
    Mat4 scaler = scale(Identity<Mat4>(), Vec3{s});
    Mat4 mover = translate(Identity<Mat4>(), b.origin - a.origin);
    return mover * scaler;
}

Transform osc::SphereToSphereTransform(Sphere const& a, Sphere const& b)
{
    Transform t;
    t.scale *= (b.radius / a.radius);
    t.position = b.origin - a.origin;
    return t;
}

AABB osc::ToAABB(Sphere const& s)
{
    AABB rv{};
    rv.min = s.origin - s.radius;
    rv.max = s.origin + s.radius;
    return rv;
}

Line osc::TransformLine(Line const& l, Mat4 const& m)
{
    Line rv{};
    rv.direction = m * Vec4{l.direction, 0.0f};
    rv.origin = m * Vec4{l.origin, 1.0f};
    return rv;
}

Line osc::InverseTransformLine(Line const& l, Transform const& t)
{
    return Line
    {
        InverseTransformPoint(t, l.origin),
        InverseTransformDirection(t, l.direction),
    };
}

Mat4 osc::DiscToDiscMat4(Disc const& a, Disc const& b)
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

    Vec3 scalers = 1.0f + ((s - 1.0f) * abs(1.0f - a.normal));
    Mat4 scaler = scale(Identity<Mat4>(), scalers);

    float cosTheta = dot(a.normal, b.normal);
    Mat4 rotator;
    if (cosTheta > 0.9999f)
    {
        rotator = Identity<Mat4>();
    }
    else
    {
        Radians theta = acos(cosTheta);
        Vec3 axis = cross(a.normal, b.normal);
        rotator = Rotate(Identity<Mat4>(), theta, axis);
    }

    Mat4 translator = translate(Identity<Mat4>(), b.origin-a.origin);

    return translator * rotator * scaler;
}

AABB osc::InvertedAABB()
{
    AABB rv{};
    rv.min = {std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max()};
    rv.max = {std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest()};
    return rv;
}

Vec3 osc::Midpoint(AABB const& a)
{
    return 0.5f * (a.min + a.max);
}

Vec3 osc::Dimensions(AABB const& a)
{
    return a.max - a.min;
}

Vec3 osc::HalfWidths(AABB const& a)
{
    return 0.5f*Dimensions(a);
}

float osc::Volume(AABB const& a)
{
    Vec3 d = Dimensions(a);
    return d.x * d.y * d.z;
}

AABB osc::Union(AABB const& a, AABB const& b)
{
    return AABB
    {
        elementwise_min(a.min, b.min),
        elementwise_max(a.max, b.max),
    };
}

bool osc::IsAPoint(AABB const& a)
{
    return a.min == a.max;
}

bool osc::IsZeroVolume(AABB const& a)
{

    for (Vec3::size_type i = 0; i < 3; ++i)
    {
        if (a.min[i] == a.max[i])
        {
            return true;
        }
    }
    return false;
}

Vec3::size_type osc::LongestDimIndex(AABB const& a)
{
    return LongestDimIndex(Dimensions(a));
}

float osc::LongestDim(AABB const& a)
{
    Vec3 const dims = Dimensions(a);
    return std::max({dims[0], dims[1], dims[2]});
}

std::array<Vec3, 8> osc::ToCubeVerts(AABB const& aabb)
{
    Vec3 d = Dimensions(aabb);

    std::array<Vec3, 8> rv{};
    rv[0] = aabb.min;
    rv[1] = aabb.max;
    size_t pos = 2;
    for (Vec3::size_type i = 0; i < 3; ++i)
    {
        Vec3 min = aabb.min;
        min[i] += d[i];
        Vec3 max = aabb.max;
        max[i] -= d[i];
        rv[pos++] = min;
        rv[pos++] = max;
    }
    return rv;
}

AABB osc::TransformAABB(AABB const& aabb, Mat4 const& m)
{
    auto verts = ToCubeVerts(aabb);

    for (Vec3& vert : verts)
    {
        Vec4 p = m * Vec4{vert, 1.0f};
        vert = Vec3{p / p.w}; // perspective divide
    }

    return AABBFromVerts(verts);
}

AABB osc::TransformAABB(AABB const& aabb, Transform const& t)
{
    // from real-time collision detection (the book)
    //
    // screenshot: https://twitter.com/Herschel/status/1188613724665335808

    Mat3 const m = ToMat3(t);

    AABB rv{t.position, t.position};  // add in the translation
    for (Vec3::size_type i = 0; i < 3; ++i)
    {
        // form extent by summing smaller and larger terms repsectively
        for (Vec3::size_type j = 0; j < 3; ++j)
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

AABB osc::AABBFromTriangle(Triangle const& t)
{
    AABB rv{t.p0, t.p0};
    rv.min = elementwise_min(rv.min, t.p1);
    rv.max = elementwise_max(rv.max, t.p1);
    rv.min = elementwise_min(rv.min, t.p2);
    rv.max = elementwise_max(rv.max, t.p2);
    return rv;
}

AABB osc::AABBFromVerts(std::span<Vec3 const> vs)
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
        Vec3 const& pos = vs[i];
        rv.min = elementwise_min(rv.min, pos);
        rv.max = elementwise_max(rv.max, pos);
    }

    return rv;
}

AABB osc::AABBFromIndexedVerts(
    std::span<Vec3 const> verts,
    std::span<uint32_t const> indices)
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
            Vec3 const& pos = verts[idx];
            rv.min = elementwise_min(rv.min, pos);
            rv.max = elementwise_max(rv.max, pos);
        }
    }

    return rv;
}

AABB osc::AABBFromIndexedVerts(
    std::span<Vec3 const> verts,
    std::span<uint16_t const> indices)
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
            Vec3 const& pos = verts[idx];
            rv.min = elementwise_min(rv.min, pos);
            rv.max = elementwise_max(rv.max, pos);
        }
    }

    return rv;
}

std::optional<Rect> osc::AABBToScreenNDCRect(
    AABB const& aabb,
    Mat4 const& viewMat,
    Mat4 const& projMat,
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
    viewspaceAABB.min.z = clamp(viewspaceAABB.min.z, -zfar, -znear);
    viewspaceAABB.max.z = clamp(viewspaceAABB.max.z, -zfar, -znear);

    // transform it into an NDC-aligned NDC-space AABB
    AABB ndcAABB = TransformAABB(viewspaceAABB, projMat);

    // take the X and Y coordinates of that AABB and ensure they are clamped to within bounds
    Rect rv{Vec2{ndcAABB.min}, Vec2{ndcAABB.max}};
    rv.p1 = clamp(rv.p1, {-1.0f, -1.0f}, {1.0f, 1.0f});
    rv.p2 = clamp(rv.p2, { -1.0f, -1.0f }, {1.0f, 1.0f});

    return rv;
}

Mat4 osc::SegmentToSegmentMat4(Segment const& a, Segment const& b)
{
    Vec3 a1ToA2 = a.p2 - a.p1;
    Vec3 b1ToB2 = b.p2 - b.p1;

    float aLen = length(a1ToA2);
    float bLen = length(b1ToB2);

    Vec3 aDir = a1ToA2 / aLen;
    Vec3 bDir = b1ToB2 / bLen;

    Vec3 aCenter = (a.p1 + a.p2)/2.0f;
    Vec3 bCenter = (b.p1 + b.p2)/2.0f;

    // this is essentially LERPing [0,1] onto [1, l] to rescale only
    // along the line's original direction
    float s = bLen/aLen;
    Vec3 scaler = Vec3{1.0f, 1.0f, 1.0f} + (s-1.0f)*aDir;

    Mat4 rotate = Dir1ToDir2Xform(aDir, bDir);
    Mat4 move = translate(Identity<Mat4>(), bCenter - aCenter);

    return move * rotate * scale(Identity<Mat4>(), scaler);
}

Transform osc::SegmentToSegmentTransform(Segment const& a, Segment const& b)
{
    Vec3 aLine = a.p2 - a.p1;
    Vec3 bLine = b.p2 - b.p1;

    float aLen = length(aLine);
    float bLen = length(bLine);

    Vec3 aDir = aLine / aLen;
    Vec3 bDir = bLine / bLen;

    Vec3 aMid = (a.p1 + a.p2)/2.0f;
    Vec3 bMid = (b.p1 + b.p2)/2.0f;

    // for scale: LERP [0,1] onto [1,l] along original direction
    Transform t;
    t.rotation = rotation(aDir, bDir);
    t.scale = Vec3{1.0f, 1.0f, 1.0f} + ((bLen/aLen - 1.0f)*aDir);
    t.position = bMid - aMid;

    return t;
}

Transform osc::YToYCylinderToSegmentTransform(Segment const& s, float radius)
{
    Segment cylinderLine{{0.0f, -1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};
    Transform t = SegmentToSegmentTransform(cylinderLine, s);
    t.scale.x = radius;
    t.scale.z = radius;
    return t;
}

Transform osc::YToYConeToSegmentTransform(Segment const& s, float radius)
{
    return YToYCylinderToSegmentTransform(s, radius);
}

Mat3 osc::ToMat3(Transform const& t)
{
    Mat3 rv = mat3_cast(t.rotation);

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

Mat4 osc::ToMat4(Transform const& t)
{
    Mat4 rv = mat4_cast(t.rotation);

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

Mat4 osc::ToInverseMat4(Transform const& t)
{
    Mat4 translater = translate(Identity<Mat4>(), -t.position);
    Mat4 rotater = mat4_cast(conjugate(t.rotation));
    Mat4 scaler = scale(Identity<Mat4>(), 1.0f/t.scale);

    return scaler * rotater * translater;
}

Mat3 osc::ToNormalMatrix(Transform const& t)
{
    return ToAdjugateMatrix(transpose(ToMat3(t)));
}

Mat4 osc::ToNormalMatrix4(Transform const& t)
{
    return ToAdjugateMatrix(transpose(ToMat3(t)));
}

Transform osc::ToTransform(Mat4 const& mtx)
{
    Transform rv;
    Vec3 skew;
    Vec4 perspective;
    if (!decompose(mtx, rv.scale, rv.rotation, rv.position, skew, perspective))
    {
        throw std::runtime_error{"failed to decompose a matrix into scale, rotation, etc."};
    }
    return rv;
}

Vec3 osc::TransformDirection(Transform const& t, Vec3 const& localDir)
{
    return normalize(t.rotation * (t.scale * localDir));
}

Vec3 osc::InverseTransformDirection(Transform const& t, Vec3 const& direction)
{
    return normalize((conjugate(t.rotation) * direction) / t.scale);
}

Vec3 osc::TransformPoint(Transform const& t, Vec3 const& p)
{
    Vec3 rv = p;
    rv *= t.scale;
    rv = t.rotation * rv;
    rv += t.position;
    return rv;
}

Vec3 osc::TransformPoint(Mat4 const& m, Vec3 const& p)
{
    return Vec3{m * Vec4{p, 1.0f}};
}

Vec3 osc::InverseTransformPoint(Transform const& t, Vec3 const& p)
{
    Vec3 rv = p;
    rv -= t.position;
    rv = conjugate(t.rotation) * rv;
    rv /= t.scale;
    return rv;
}

Quat osc::WorldspaceRotation(Eulers const& eulers)
{
    static_assert(std::is_same_v<Eulers::value_type, Radians>);
    return normalize(Quat{Vec3{eulers.x.count(), eulers.y.count(), eulers.z.count()}});
}

void osc::ApplyWorldspaceRotation(
    Transform& t,
    Eulers const& eulerAngles,
    Vec3 const& rotationCenter)
{
    Quat q = WorldspaceRotation(eulerAngles);
    t.position = q*(t.position - rotationCenter) + rotationCenter;
    t.rotation = normalize(q*t.rotation);
}

Eulers osc::ExtractEulerAngleXYZ(Transform const& t)
{
    return extract_eulers_xyz(mat4_cast(t.rotation));
}

Eulers osc::ExtractExtrinsicEulerAnglesXYZ(Transform const& t)
{
    return euler_angles(t.rotation);
}

Transform osc::PointAxisAlong(Transform const& t, int axisIndex, Vec3 const& newDirection)
{
    Vec3 beforeDir{};
    beforeDir[axisIndex] = 1.0f;
    beforeDir = t.rotation * beforeDir;

    Quat const rotBeforeToAfter = Rotation(beforeDir, newDirection);
    Quat const newRotation = normalize(rotBeforeToAfter * t.rotation);

    return t.withRotation(newRotation);
}

Transform osc::PointAxisTowards(Transform const& t, int axisIndex, Vec3 const& location)
{
    return PointAxisAlong(t, axisIndex, normalize(location - t.position));
}

Transform osc::RotateAlongAxis(Transform const& t, int axisIndex, Radians angle)
{
    Vec3 ax{};
    ax[axisIndex] = 1.0f;
    ax = t.rotation * ax;

    Quat const q = AngleAxis(angle, ax);

    return t.withRotation(normalize(q * t.rotation));
}

bool osc::IsPointInRect(Rect const& r, Vec2 const& p)
{
    Vec2 relPos = p - r.p1;
    Vec2 dims = Dimensions(r);
    return (0.0f <= relPos.x && relPos.x <= dims.x) && (0.0f <= relPos.y && relPos.y <= dims.y);
}

std::optional<RayCollision> osc::GetRayCollisionSphere(Line const& l, Sphere const& s)
{
    return GetRayCollisionSphereAnalytic(s, l);
}

std::optional<RayCollision> osc::GetRayCollisionAABB(Line const& l, AABB const& bb)
{
    // intersect the ray with each axis-aligned slab for each dimension
    //
    // i.e. figure out where the line intersects the front+back of the AABB
    //      in (e.g.) X, then Y, then Z, and intersect those interactions such
    //      that if the intersection is ever empty (or, negative here) then there
    //      is no intersection
    float t0 = std::numeric_limits<float>::lowest();
    float t1 = std::numeric_limits<float>::max();
    for (Vec3::size_type i = 0; i < 3; ++i)
    {
        float invDir = 1.0f / l.direction[i];
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

    return RayCollision{t0, l.origin + t0*l.direction};
}

std::optional<RayCollision> osc::GetRayCollisionPlane(Line const& l, Plane const& p)
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

    float denominator = dot(p.normal, l.direction);

    if (std::abs(denominator) > 1e-6)
    {
        float numerator = dot(p.origin - l.origin, p.normal);
        float distance = numerator / denominator;
        return RayCollision{distance, l.origin + distance*l.direction};
    }
    else
    {
        // the line is *very* parallel to the plane, which could cause
        // some divide-by-zero havok: pretend it didn't intersect
        return std::nullopt;
    }
}

std::optional<RayCollision> osc::GetRayCollisionDisc(Line const& l, Disc const& d)
{
    // see: https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-plane-and-ray-disk-intersection

    // think of this as a ray-plane intersection test with the additional
    // constraint that the ray has to be within the radius of the disc

    Plane p{};
    p.origin = d.origin;
    p.normal = d.normal;

    std::optional<RayCollision> maybePlaneCollision = GetRayCollisionPlane(l, p);

    if (!maybePlaneCollision)
    {
        return std::nullopt;
    }

    // figure out whether the plane hit is within the disc's radius
    Vec3 v = maybePlaneCollision->position - d.origin;
    float const d2 = dot(v, v);
    float const r2 = dot(d.radius, d.radius);

    if (d2 > r2)
    {
        return std::nullopt;
    }

    return maybePlaneCollision;
}

std::optional<RayCollision> osc::GetRayCollisionTriangle(Line const& l, Triangle const& tri)
{
    // see: https://www.scratchapixel.com/lessons/3d-basic-rendering/ray-tracing-rendering-a-triangle/ray-triangle-intersection-geometric-solution

    // compute triangle normal
    Vec3 N = TriangleNormal(tri);

    // compute dot product between normal and ray
    float NdotR = dot(N, l.direction);

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
    float D = dot(N, tri.p0);

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
    float t = -(dot(N, l.origin) - D) / NdotR;

    // if triangle plane is behind line then return early
    if (t < 0.0f)
    {
        return std::nullopt;
    }

    // intersection point on triangle plane, computed from line equation
    Vec3 P = l.origin + t*l.direction;

    // figure out if that point is inside the triangle's bounds using the
    // "inside-outside" test

    // test each triangle edge: {0, 1}, {1, 2}, {2, 0}
    for (size_t i = 0; i < 3; ++i)
    {
        Vec3 const& start = tri[i];
        Vec3 const& end = tri[(i+1)%3];

        // corner[n] to corner[n+1]
        Vec3 e = end - start;

        // corner[n] to P
        Vec3 c = P - start;

        // cross product of the above indicates whether the vectors are
        // clockwise or anti-clockwise with respect to eachover. It's a
        // right-handed coord system, so anti-clockwise produces a vector
        // that points in same direction as normal
        Vec3 ax = cross(e, c);

        // if the dot product of that axis with the normal is <0.0f then
        // the point was "outside"
        if (dot(ax, N) < 0.0f)
        {
            return std::nullopt;
        }
    }

    return RayCollision{t, l.origin + t*l.direction};
}

float osc::EaseOutElastic(float x)
{
    // adopted from: https://easings.net/#easeOutElastic

    using std::pow;

    constexpr float c4 = 2.0f*std::numbers::pi_v<float> / 3.0f;
    float const normalized = saturate(x);

    return pow(2.0f, -5.0f*normalized) * sin((normalized*10.0f - 0.75f) * c4) + 1.0f;
}

// FrameAxis
namespace
{
    std::string_view ToStringView(FrameAxis fa)
    {
        static_assert(NumOptions<FrameAxis>() == 6);

        switch (fa)
        {
        case FrameAxis::PlusX: return "x";
        case FrameAxis::PlusY: return "y";
        case FrameAxis::PlusZ: return "z";
        case FrameAxis::MinusX: return "-x";
        case FrameAxis::MinusY: return "-y";
        case FrameAxis::MinusZ: return "-z";
        default:
            return "unknown";
        }
    }
}

std::optional<FrameAxis> osc::TryParseAsFrameAxis(std::string_view s)
{
    if (s.empty() || s.size() > 2)
    {
        return std::nullopt;
    }
    bool const negated = s.front() == '-';
    if (negated || s.front() == '+')
    {
        s = s.substr(1);
    }
    if (s.empty())
    {
        return std::nullopt;
    }
    switch (s.front())
    {
    case 'x':
    case 'X':
        return negated ? FrameAxis::MinusX : FrameAxis::PlusX;
    case 'y':
    case 'Y':
        return negated ? FrameAxis::MinusY : FrameAxis::PlusY;
    case 'z':
    case 'Z':
        return negated ? FrameAxis::MinusZ : FrameAxis::PlusZ;
    default:
        return std::nullopt;  // invalid input
    }
}

bool osc::AreOrthogonal(FrameAxis a, FrameAxis b)
{
    static_assert(cpp23::to_underlying(FrameAxis::PlusX) == 0);
    static_assert(cpp23::to_underlying(FrameAxis::MinusX) == 3);
    static_assert(NumOptions<FrameAxis>() == 6);
    return cpp23::to_underlying(a) % 3 != cpp23::to_underlying(b) % 3;
}

std::ostream& osc::operator<<(std::ostream& o, FrameAxis f)
{
    return o << ToStringView(f);
}
