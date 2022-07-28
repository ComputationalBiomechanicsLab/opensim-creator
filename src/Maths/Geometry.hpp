#pragma once

#include "src/Maths/AABB.hpp"
#include "src/Maths/Line.hpp"
#include "src/Maths/RayCollision.hpp"
#include "src/Maths/Sphere.hpp"
#include "src/Maths/Transform.hpp"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x3.hpp>
#include <glm/mat4x4.hpp>
#include <nonstd/span.hpp>

#include <array>
#include <cstdint>
#include <cstddef>

namespace osc
{
    struct Disc;
    struct Plane;
    struct Rect;
    struct Segment;
}

// geometry: low-level, backend-independent, geometric maths
namespace osc
{
    // returns true if the provided vectors are at the same location
    bool AreAtSameLocation(glm::vec3 const&, glm::vec3 const&) noexcept;

    // returns a vector containing min(a[dim], b[dim]) for each dimension
    glm::vec3 Min(glm::vec3 const&, glm::vec3 const&) noexcept;

    // returns a vector containing min(a[dim], b[dim]) for each dimension
    glm::vec2 Min(glm::vec2 const&, glm::vec2 const&) noexcept;

    // returns a vector containing max(a[dim], b[dim]) for each dimension
    glm::vec3 Max(glm::vec3 const&, glm::vec3 const&) noexcept;

    // returns a vector containing max(a[dim], b[dim]) for each dimension
    glm::vec2 Max(glm::vec2 const&, glm::vec2 const&) noexcept;

    // returns the *index* of a vector's longest dimension
    glm::vec3::length_type LongestDimIndex(glm::vec3 const&) noexcept;

    // returns the *index* of a vector's longest dimension
    glm::vec2::length_type LongestDimIndex(glm::vec2) noexcept;

    // returns the *index* of a vector's longest dimension
    glm::ivec2::length_type LongestDimIndex(glm::ivec2) noexcept;

    // returns the *value* of a vector's longest dimension
    float LongestDim(glm::vec3 const&) noexcept;

    // returns the *value* of a vector's longest dimension
    float LongestDim(glm::vec2) noexcept;

    // returns the *value* of a vector's longest dimension
    int LongestDim(glm::ivec2) noexcept;

    // returns the aspect ratio of the vec (effectively: x/y)
    float AspectRatio(glm::ivec2) noexcept;

    // returns the aspect ratio of the vec (effectively: x/y)
    float AspectRatio(glm::vec2) noexcept;

    // returns the midpoint between two vectors (effectively: (x+y)/2.0)
    glm::vec3 Midpoint(glm::vec3 const&, glm::vec3 const&) noexcept;

    // returns the unweighted midpoint of all of the provided vectors, or {0.0f, 0.0f, 0.0f} if provided none
    glm::vec3 Midpoint(nonstd::span<glm::vec3 const>) noexcept;

    // returns the sum of `n` vectors using the "Kahan Summation Algorithm" to reduce errors
    glm::vec3 KahanSum(glm::vec3 const*, size_t n) noexcept;

    // returns the average of `n` vectors using whichever numerically stable average happens to work
    glm::vec3 NumericallyStableAverage(glm::vec3 const*, size_t n) noexcept;

    // returns a normal vector of the supplied (pointed to) triangle (i.e. (v[1]-v[0]) x (v[2]-v[0]))
    glm::vec3 TriangleNormal(glm::vec3 const*) noexcept;

    // returns a normal vector of the supplied triangle (i.e. (B-A) x (C-A))
    glm::vec3 TriangleNormal(glm::vec3 const&, glm::vec3 const&, glm::vec3 const&) noexcept;

    // returns a normal matrix created from the supplied xform matrix
    glm::mat3 ToNormalMatrix(glm::mat4 const&) noexcept;

    // returns a normal matrix created from the supplied xform matrix
    glm::mat3 ToNormalMatrix(glm::mat4x3 const&) noexcept;

    // returns matrix that rotates dir1 to point in the same direction as dir2
    glm::mat4 Dir1ToDir2Xform(glm::vec3 const& dir1, glm::vec3 const& dir2) noexcept;

    // returns euler angles for performing an intrinsic, step-by-step, rotation about X, Y, and then Z
    glm::vec3 ExtractEulerAngleXYZ(glm::mat4 const&) noexcept;

    // returns the area of the rectangle
    float Area(Rect const&) noexcept;

    // returns the edge dimensions of the rectangle
    glm::vec2 Dimensions(Rect const&) noexcept;

    // returns bottom-left point of the rectangle
    glm::vec2 BottomLeft(Rect const&) noexcept;

    // returns the aspect ratio (width/height) of the rectangle
    float AspectRatio(Rect const&) noexcept;

    // returns true if the given point is within the rect's bounds
    bool IsPointInRect(Rect const&, glm::vec2 const&) noexcept;

    // returns a sphere that bounds the given vertices
    Sphere BoundingSphereOf(glm::vec3 const*, size_t n) noexcept;

    // returns an xform that maps an origin centered r=1 sphere into an in-scene sphere
    glm::mat4 FromUnitSphereMat4(Sphere const&) noexcept;

    // returns an xform that maps a sphere to another sphere
    glm::mat4 SphereToSphereMat4(Sphere const&, Sphere const&) noexcept;

    // returns an AABB that contains the sphere
    AABB ToAABB(Sphere const&) noexcept;

    // returns a line that has been transformed by the supplied transform matrix
    Line TransformLine(Line const&, glm::mat4 const&) noexcept;

    // returns an xform that maps a disc to another disc
    glm::mat4 DiscToDiscMat4(Disc const&, Disc const&) noexcept;

    // returns the centerpoint of an AABB
    glm::vec3 Midpoint(AABB const&) noexcept;

    // returns the dimensions of an AABB
    glm::vec3 Dimensions(AABB const&) noexcept;

    // returns the volume of the AABB
    float Volume(AABB const&) noexcept;

    // returns the smallest AABB that spans both of the provided AABBs
    AABB Union(AABB const&, AABB const&) noexcept;

    // advanced: returns the smallest AABB that spans all the AABBs at `offset` from the start of data with size `stride`
    AABB Union(void const* data, size_t n, size_t stride, size_t offset);

    // returns true if the AABB has an effective volume of 0
    bool IsEffectivelyEmpty(AABB const&) noexcept;

    // returns the *index* of the longest dimension of an AABB
    glm::vec3::length_type LongestDimIndex(AABB const&) noexcept;

    // returns the length of the longest dimension of an AABB
    float LongestDim(AABB const&) noexcept;

    // returns the eight corner points of the cuboid representation of the AABB
    std::array<glm::vec3, 8> ToCubeVerts(AABB const&) noexcept;

    // apply a transformation matrix to the AABB
    //
    // note: don't do this repeatably, because it can keep growing the AABB
    AABB TransformAABB(AABB const&, glm::mat4 const&) noexcept;
    AABB TransformAABB(AABB const&, Transform const&) noexcept;

    // computes an AABB of free-floating points in space
    AABB AABBFromVerts(glm::vec3 const*, size_t n) noexcept;
    AABB AABBFromVerts(nonstd::span<glm::vec3 const>) noexcept;

    // computes an AABB of indexed verticies (e.g. as used in mesh data)
    AABB AABBFromIndexedVerts(nonstd::span<glm::vec3 const> verts, nonstd::span<uint32_t const> indices);
    AABB AABBFromIndexedVerts(nonstd::span<glm::vec3 const> verts, nonstd::span<uint16_t const> indices);

    // returns an xform that maps a path segment to another path segment
    glm::mat4 SegmentToSegmentMat4(Segment const&, Segment const&) noexcept;

    // returns a transform that maps a path segment to another path segment
    Transform SegmentToSegmentTransform(Segment const&, Segment const&) noexcept;

    // collision tests
    RayCollision GetRayCollisionSphere(Line const&, Sphere const&) noexcept;
    RayCollision GetRayCollisionAABB(Line const&, AABB const&) noexcept;
    RayCollision GetRayCollisionPlane(Line const&, Plane const&) noexcept;
    RayCollision GetRayCollisionDisc(Line const&, Disc const&) noexcept;
    RayCollision GetRayCollisionTriangle(Line const&, glm::vec3 const*) noexcept;

    // returns a transform that maps a simbody standard cylinder to a segment with the given radius
    Transform SimbodyCylinderToSegmentTransform(Segment const&, float radius) noexcept;

    // returns a transform that maps a simbody standard cone to a segment with the given radius
    Transform SimbodyConeToSegmentTransform(Segment const&, float radius) noexcept;

    // converts a topleft-origin RELATIVE `pos` (0 to 1 in XY starting topleft) into an
    // XY location in NDC (-1 to +1 in XY starting in the middle)
    glm::vec2 TopleftRelPosToNDCPoint(glm::vec2 relpos);

    // converts a topleft-origin RELATIVE `pos` (0 to 1 in XY, starting topleft) into
    // the equivalent POINT on the front of the NDC cube (i.e. "as if" a viewer was there)
    //
    // i.e. {X_ndc, Y_ndc, -1.0f, 1.0f}
    glm::vec4 TopleftRelPosToNDCCube(glm::vec2 relpos);

    // converts a `Transform` to a standard 4x4 transform matrix
    glm::mat4 ToMat4(Transform const&) noexcept;

    // inverses `Transform` and converts it to a standard 4x4 transformation matrix
    glm::mat4 ToInverseMat4(Transform const&) noexcept;

    // converts a `Transform` to a normal matrix
    glm::mat3 ToNormalMatrix(Transform const&) noexcept;

    // decomposes the provided transform matrix into a `Transform`, throws if decomposition is not possible
    Transform ToTransform(glm::mat4 const&);

    // transforms the direction of a vector
    //
    // not affected by scale or position of the transform. The returned vector has the
    // same length as the provided vector
    glm::vec3 TransformDirection(Transform const&, glm::vec3 const&) noexcept;

    // inverse-transforms the direction of a vector
    //
    // not affected by scale or position of the transform. The returned vector has the same
    // length as the provided vector
    glm::vec3 InverseTransformDirection(Transform const&, glm::vec3 const&) noexcept;

    // transforms a point
    //
    // the returned point is affected by the position, rotation, and scale of the transform.
    glm::vec3 TransformPoint(Transform const&, glm::vec3 const&) noexcept;

    // inverse-transforms a point
    //
    // the returned point is affected by the position, rotation, and scale of the transform.
    glm::vec3 InverseTransformPoint(Transform const&, glm::vec3 const&) noexcept;

    // applies a world-space rotation to the transform
    void ApplyWorldspaceRotation(Transform& applicationTarget, glm::vec3 const& eulerAngles, glm::vec3 const& rotationCenter) noexcept;

    // returns XYZ (pitch, yaw, roll) Euler angles for a one-by-one application of an
    // intrinsic rotations. These rotations are what (e.g.) OpenSim uses for joint
    // coordinates.
    //
    // Each rotation is applied one-at-a-time, to the transformed space, so we have:
    //
    // x-y-z (initial)
    // x'-y'-z' (after first rotation)
    // x''-y''-z'' (after second rotation)
    // x'''-y'''-z''' (after third rotation)
    //
    // Assuming we're doing an XYZ rotation, the first rotation rotates x, the second
    // rotation rotates around y', and the third rotation rotates around z''
    //
    // see: https://en.wikipedia.org/wiki/Euler_angles#Conventions_by_intrinsic_rotations
    glm::vec3 ExtractEulerAngleXYZ(Transform const&) noexcept;

    // returns XYZ (pitch, yaw, roll) Euler angles for an extrinsic rotation
    //
    // in extrinsic rotations, each rotation happens about a *fixed* coordinate system, which
    // is in contrast to intrinsic rotations, which happen in a coordinate system that's attached
    // to a moving body (the thing being rotated)
    //
    // see: https://en.wikipedia.org/wiki/Euler_angles#Conventions_by_extrinsic_rotations
    glm::vec3 ExtractExtrinsicEulerAnglesXYZ(Transform const&) noexcept;
}
