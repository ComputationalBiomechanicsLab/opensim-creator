#pragma once

#include "oscar/Maths/AABB.hpp"
#include "oscar/Maths/Line.hpp"
#include "oscar/Maths/Sphere.hpp"
#include "oscar/Maths/Transform.hpp"
#include "oscar/Maths/Triangle.hpp"

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
#include <optional>

namespace osc { struct Disc; }
namespace osc { struct Plane; }
namespace osc { struct Rect; }
namespace osc { struct Segment; }

// math helpers: generally handy math functions that aren't attached to a particular
//               osc struct
namespace osc
{
    // returns `true` if the two arguments are effectively equal (i.e. within machine precision)
    //
    // this algorithm is designed to be correct, rather than fast
    bool IsEffectivelyEqual(double, double) noexcept;

    bool IsLessThanOrEffectivelyEqual(double, double) noexcept;

    // returns `true` if the first two arguments are within `relativeError` of eachover
    bool IsEqualWithinRelativeError(float, float, float relativeError) noexcept;
    bool IsEqualWithinRelativeError(glm::vec3 const&, glm::vec3 const&, float relativeError) noexcept;

    // ----- glm::vecX/glm::matX helpers -----

    // returns true if the provided vectors are at the same location
    bool AreAtSameLocation(glm::vec3 const&, glm::vec3 const&) noexcept;

    // returns a vector containing min(a[dim], b[dim]) for each dimension
    glm::vec3 Min(glm::vec3 const&, glm::vec3 const&) noexcept;

    // returns a vector containing min(a[dim], b[dim]) for each dimension
    glm::vec2 Min(glm::vec2 const&, glm::vec2 const&) noexcept;

    // returns a vector containing min(a[dim], b[dim]) for each dimension
    glm::ivec2 Min(glm::ivec2 const&, glm::ivec2 const&) noexcept;

    // returns a vector containing max(a[dim], b[dim]) for each dimension
    glm::vec3 Max(glm::vec3 const&, glm::vec3 const&) noexcept;

    // returns a vector containing max(a[dim], b[dim]) for each dimension
    glm::vec2 Max(glm::vec2 const&, glm::vec2 const&) noexcept;

    // returns a vector containing min(a[dim], b[dim]) for each dimension
    glm::ivec2 Max(glm::ivec2 const&, glm::ivec2 const&) noexcept;

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
    glm::ivec2::value_type LongestDim(glm::ivec2) noexcept;

    // returns the aspect ratio of the vec (effectively: x/y)
    float AspectRatio(glm::ivec2) noexcept;

    // returns the aspect ratio of the vec (effectively: x/y)
    float AspectRatio(glm::vec2) noexcept;

    // returns the midpoint between two vectors (effectively: 0.5 * (a+b))
    glm::vec2 Midpoint(glm::vec2, glm::vec2) noexcept;

    // returns the midpoint between two vectors (effectively: 0.5 * (a+b))
    glm::vec3 Midpoint(glm::vec3 const&, glm::vec3 const&) noexcept;

    // returns the unweighted midpoint of all of the provided vectors, or {0.0f, 0.0f, 0.0f} if provided no inputs
    glm::vec3 Midpoint(nonstd::span<glm::vec3 const>) noexcept;

    // returns the sum of `n` vectors using the "Kahan Summation Algorithm" to reduce errors, returns {0.0f, 0.0f, 0.0f} if provided no inputs
    glm::vec3 KahanSum(nonstd::span<glm::vec3 const>) noexcept;

    // returns the average of `n` vectors using whichever numerically stable average happens to work
    glm::vec3 NumericallyStableAverage(nonstd::span<glm::vec3 const>) noexcept;

    // returns a normal vector of the supplied (pointed to) triangle (i.e. (v[1]-v[0]) x (v[2]-v[0]))
    glm::vec3 TriangleNormal(Triangle const&) noexcept;

    // returns the adjugate matrix of the given 3x3 input
    glm::mat3 ToAdjugateMatrix(glm::mat3 const&) noexcept;

    // returns a normal matrix created from the supplied xform matrix
    glm::mat3 ToNormalMatrix(glm::mat4 const&) noexcept;

    // returns a normal matrix created from the supplied xform matrix
    glm::mat3 ToNormalMatrix(glm::mat4x3 const&) noexcept;

    // returns a noraml matrix created from the supplied xform matrix
    //
    // equivalent to mat4{ToNormalMatrix(m)}
    glm::mat4 ToNormalMatrix4(glm::mat4 const&) noexcept;

    // returns a transform matrix that rotates dir1 to point in the same direction as dir2
    glm::mat4 Dir1ToDir2Xform(glm::vec3 const& dir1, glm::vec3 const& dir2) noexcept;

    // returns euler angles for performing an intrinsic, step-by-step, rotation about X, Y, and then Z
    glm::vec3 ExtractEulerAngleXYZ(glm::quat const&) noexcept;

    // returns euler angles for performing an intrinsic, step-by-step, rotation about X, Y, and then Z
    glm::vec3 ExtractEulerAngleXYZ(glm::mat4 const&) noexcept;

    // returns an XY NDC point converted from a screen/viewport point
    //
    // - input screen point has origin in top-left, Y goes down
    // - input screen point has range: (0,0) is top-left, (1, 1) is bottom-right
    // - output NDC point has origin in middle, Y goes up
    // - output NDC point has range: (-1, 1) is top-left, (1, -1) is bottom-right
    glm::vec2 TopleftRelPosToNDCPoint(glm::vec2 relpos);

    // returns an XY top-left relative point converted from the given NDC point
    //
    // - input NDC point has origin in the middle, Y goes up
    // - input NDC point has range: (-1, -1) for top-left, (1, -1) is bottom-right
    // - output point has origin in the top-left, Y goes down
    // - output point has range: (0, 0) for top-left, (1, 1) for bottom-right
    glm::vec2 NDCPointToTopLeftRelPos(glm::vec2 ndcPos);

    // returns an NDC affine point vector (i.e. {x, y, z, 1.0}) converted from a screen/viewport point
    //
    // - input screen point has origin in top-left, Y goes down
    // - input screen point has range: (0,0) is top-left, (1, 1) is bottom-right
    // - output NDC point has origin in middle, Y goes up
    // - output NDC point has range -1 to +1 in each dimension
    // - output assumes Z is "at the front of the cube" (Z = -1.0f)
    // - output will therefore be: {xNDC, yNDC, -1.0f, 1.0f}
    glm::vec4 TopleftRelPosToNDCCube(glm::vec2 relpos);

    // "un-project" a screen/viewport point into 3D world-space, assuming a
    // perspective camera
    //
    // - input screen point has origin in top-left, Y goes down
    // - input screen point has range: (0,0) is top-left, (1, 1) is bottom-right
    // - `cameraWorldspaceOrigin` is the location of the camera in world space
    // - `cameraViewMatrix` transforms points from world-space to view-space
    // - `cameraProjMatrix` transforms points from view-space to world-space
    Line PerspectiveUnprojectTopLeftScreenPosToWorldRay(
        glm::vec2 relpos,
        glm::vec3 cameraWorldspaceOrigin,
        glm::mat4 const& cameraViewMatrix,
        glm::mat4 const& cameraProjMatrix
    );

    // ----- `Rect` helpers -----

    // returns `Min(rect.p1, rect.p2)`: i.e. the smallest X and the smallest Y of the rectangle's points
    glm::vec2 MinValuePerDimension(Rect const&) noexcept;

    // returns the area of the rectangle
    float Area(Rect const&) noexcept;

    // returns the dimensions of the rectangle
    glm::vec2 Dimensions(Rect const&) noexcept;

    // returns bottom-left point of the rectangle
    glm::vec2 BottomLeft(Rect const&) noexcept;

    // returns the aspect ratio (width/height) of the rectangle
    float AspectRatio(Rect const&) noexcept;

    // returns the middle point of the rectangle
    glm::vec2 Midpoint(Rect const&) noexcept;

    // returns a rectangle that has been expanded along each edge by the given amount
    //
    // (e.g. expand 1.0f adds 1.0f to both the left edge and the right edge)
    Rect Expand(Rect const&, float) noexcept;
    Rect Expand(Rect const&, glm::vec2) noexcept;

    // returns a rectangle where both p1 and p2 are clamped between min and max (inclusive)
    Rect Clamp(Rect const&, glm::vec2 const& min, glm::vec2 const& max) noexcept;

    // returns a rect, created by mapping an Normalized Device Coordinates (NDC) rect
    // (i.e. -1.0 to 1.0) within a screenspace viewport (pixel units, topleft == (0, 0))
    Rect NdcRectToScreenspaceViewportRect(Rect const& ndcRect, Rect const& viewport) noexcept;


    // ----- `Sphere` helpers -----

    // returns a sphere that bounds the given vertices
    Sphere BoundingSphereOf(nonstd::span<glm::vec3 const>) noexcept;

    // returns a sphere that loosely bounds the given AABB
    Sphere ToSphere(AABB const&) noexcept;

    // returns an xform that maps an origin centered r=1 sphere into an in-scene sphere
    glm::mat4 FromUnitSphereMat4(Sphere const&) noexcept;

    // returns an xform that maps a sphere to another sphere
    glm::mat4 SphereToSphereMat4(Sphere const&, Sphere const&) noexcept;
    Transform SphereToSphereTransform(Sphere const&, Sphere const&) noexcept;

    // returns an AABB that contains the sphere
    AABB ToAABB(Sphere const&) noexcept;


    // ----- `Line` helpers -----

    // returns a line that has been transformed by the supplied transform matrix
    Line TransformLine(Line const&, glm::mat4 const&) noexcept;

    // returns a line that has been transformed by the inverse of the supplied transform
    Line InverseTransformLine(Line const&, Transform const&) noexcept;


    // ----- `Disc` helpers -----

    // returns an xform that maps a disc to another disc
    glm::mat4 DiscToDiscMat4(Disc const&, Disc const&) noexcept;


    // ----- `AABB` helpers -----

    // returns an AABB that is "inverted", such that it's minimum is the largest
    // possible value and its maximum is the smallest possible value
    //
    // handy as a "seed" when union-ing many AABBs, because it won't
    AABB InvertedAABB() noexcept;

    // returns the centerpoint of an AABB
    glm::vec3 Midpoint(AABB const&) noexcept;

    // returns the dimensions of an AABB
    glm::vec3 Dimensions(AABB const&) noexcept;

    // returns the volume of the AABB
    float Volume(AABB const&) noexcept;

    // returns the smallest AABB that spans both of the provided AABBs
    AABB Union(AABB const&, AABB const&) noexcept;

    // returns true if the AABB has no extents in any dimension
    bool IsAPoint(AABB const&) noexcept;

    // returns true if the AABB is an extent in any dimension is zero
    bool IsZeroVolume(AABB const&) noexcept;

    // returns the *index* of the longest dimension of an AABB
    glm::vec3::length_type LongestDimIndex(AABB const&) noexcept;

    // returns the length of the longest dimension of an AABB
    float LongestDim(AABB const&) noexcept;

    // returns the eight corner points of the cuboid representation of the AABB
    std::array<glm::vec3, 8> ToCubeVerts(AABB const&) noexcept;

    // returns an AABB that has been transformed by the given matrix
    AABB TransformAABB(AABB const&, glm::mat4 const&) noexcept;

    // returns an AABB that has been transformed by the given transform
    AABB TransformAABB(AABB const&, Transform const&) noexcept;

    // returns an AAB that tightly bounds the provided triangle
    AABB AABBFromTriangle(Triangle const& t) noexcept;

    // returns an AABB that tightly bounds the provided points
    AABB AABBFromVerts(nonstd::span<glm::vec3 const>) noexcept;

    // returns an AABB that tightly bounds the points indexed by the provided 32-bit indices
    AABB AABBFromIndexedVerts(nonstd::span<glm::vec3 const> verts, nonstd::span<uint32_t const> indices);

    // returns an AABB that tightly bounds the points indexed by the provided 16-bit indices
    AABB AABBFromIndexedVerts(nonstd::span<glm::vec3 const> verts, nonstd::span<uint16_t const> indices);

    // (tries to) return a Normalized Device Coordinate (NDC) rectangle, clamped to the NDC clipping
    // bounds ((-1,-1) to (1, 1)), where the rectangle loosely bounds the given worldspace AABB after
    // projecting it into NDC
    std::optional<Rect> AABBToScreenNDCRect(
        AABB const&,
        glm::mat4 const& viewMat,
        glm::mat4 const& projMat,
        float znear,
        float zfar
    );


    // ----- `Segment` helpers -----

    // returns a transform matrix that maps a path segment to another path segment
    glm::mat4 SegmentToSegmentMat4(Segment const&, Segment const&) noexcept;

    // returns a transform that maps a path segment to another path segment
    Transform SegmentToSegmentTransform(Segment const&, Segment const&) noexcept;

    // returns a transform that maps a Y-to-Y (bottom-to-top) cylinder to a segment with the given radius
    Transform YToYCylinderToSegmentTransform(Segment const&, float radius) noexcept;

    // returns a transform that maps a Y-to-Y (bottom-to-top) cone to a segment with the given radius
    Transform YToYConeToSegmentTransform(Segment const&, float radius) noexcept;


    // ----- `Transform` helpers -----

    // returns a 3x3 transform matrix equivalent to the provided transform (ignores position)
    glm::mat3 ToMat3(Transform const&) noexcept;

    // returns a 4x4 transform matrix equivalent to the provided transform
    glm::mat4 ToMat4(Transform const&) noexcept;

    // returns a 4x4 transform matrix equivalent to the inverse of the provided transform
    glm::mat4 ToInverseMat4(Transform const&) noexcept;

    // returns a 3x3 normal matrix for the provided transform
    glm::mat3 ToNormalMatrix(Transform const&) noexcept;

    // returns a 4x4 normal matrix for the provided transform
    glm::mat4 ToNormalMatrix4(Transform const&) noexcept;

    // returns a transform that *tries to* perform the equivalent transform as the provided mat4
    //
    // - not all 4x4 matrices can be expressed as an `Transform` (e.g. those containing skews)
    // - uses matrix decomposition to break up the provided matrix
    // - throws if decomposition of the provided matrix is not possible
    Transform ToTransform(glm::mat4 const&);

    // returns a unit-length vector that is the equivalent of the provided direction vector after applying the transform
    //
    // effectively, apply the Transform but ignore the `position` (translation) component
    glm::vec3 TransformDirection(Transform const&, glm::vec3 const&) noexcept;

    // returns a unit-length vector that is the equivalent of the provided direction vector after applying the inverse of the transform
    //
    // effectively, apply the inverse transform but ignore the `position` (translation) component
    glm::vec3 InverseTransformDirection(Transform const&, glm::vec3 const&) noexcept;

    // returns a vector that is the equivalent of the provided vector after applying the transform
    glm::vec3 TransformPoint(Transform const&, glm::vec3 const&) noexcept;

    // returns a vector that is the equivalent of the provided vector after applying the inverse of the transform
    glm::vec3 InverseTransformPoint(Transform const&, glm::vec3 const&) noexcept;

    // applies a world-space rotation to the transform
    void ApplyWorldspaceRotation(
        Transform& applicationTarget,
        glm::vec3 const& eulerAngles,
        glm::vec3 const& rotationCenter
    ) noexcept;

    // returns XYZ (pitch, yaw, roll) Euler angles for a one-by-one application of an
    // intrinsic rotations.
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
