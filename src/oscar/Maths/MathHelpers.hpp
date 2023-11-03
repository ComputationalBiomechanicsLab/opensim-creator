#pragma once

#include <oscar/Maths/AABB.hpp>
#include <oscar/Maths/Line.hpp>
#include <oscar/Maths/Mat3.hpp>
#include <oscar/Maths/Mat4.hpp>
#include <oscar/Maths/Mat4x3.hpp>
#include <oscar/Maths/Quat.hpp>
#include <oscar/Maths/Sphere.hpp>
#include <oscar/Maths/Transform.hpp>
#include <oscar/Maths/Triangle.hpp>
#include <oscar/Maths/Vec2.hpp>
#include <oscar/Maths/Vec3.hpp>
#include <oscar/Maths/Vec4.hpp>

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
    // converts degrees to radians
    template<typename Number>
    constexpr Number Deg2Rad(Number degrees)
    {
        return degrees * static_cast<Number>(0.01745329251994329576923690768489);
    }

    constexpr Vec3 Deg2Rad(Vec3 degrees)
    {
        return degrees * static_cast<Vec3::value_type>(0.01745329251994329576923690768489);
    }

    // converts radians to degrees
    template<typename Number>
    constexpr Number Rad2Deg(Number radians)
    {
        return radians * static_cast<Number>(57.295779513082320876798154814105);
    }

    constexpr Vec3 Rad2Deg(Vec3 radians)
    {
        return radians * static_cast<Vec3::value_type>(57.295779513082320876798154814105);
    }

    constexpr float Clamp(float v, float low, float high)
    {
        return v < low ? low : high < v ? high : v;
    }

    constexpr float Dot(Vec2 const& a, Vec2 const& b)
    {
        Vec2 tmp(a * b);
        return tmp.x + tmp.y;
    }

    constexpr float Dot(Vec3 const& a, Vec3 const& b)
    {
        Vec3 const tmp(a * b);
        return tmp.x + tmp.y + tmp.z;
    }

    Vec4 Clamp(Vec4 const&, float min, float max);

    float Mix(float a, float b, float factor);
    Vec2 Mix(Vec2 const&, Vec2 const&, float);
    Vec3 Mix(Vec3 const&, Vec3 const&, float);
    Vec4 Mix(Vec4 const&, Vec4 const&, float);

    // returns the cross product of the two arguments
    Vec3 Cross(Vec3 const&, Vec3 const&);

    // returns a normalized version of the provided argument
    Quat Normalize(Quat const&);
    Vec3 Normalize(Vec3 const&);
    Vec2 Normalize(Vec2 const&);

    // returns the length of the provided vector
    float Length(Vec2 const&);
    float Length2(Vec3 const&);

    // computes the rotation from `src` to `dest`
    Quat Rotation(Vec3 const& src, Vec3 const& dest);

    // computes a rotation from an angle+axis
    Quat AngleAxis(float angle, Vec3 const& axis);

    // computes a view matrix for the given params
    Mat4 LookAt(Vec3 const& eye, Vec3 const& center, Vec3 const& up);

    // computes a perspective projection matrix
    Mat4 Perspective(float verticalFOV, float aspectRatio, float zNear, float zFar);

    // computes an orthogonal projection matrix
    Mat4 Ortho(float left, float right, float bottom, float top, float zNear, float zFar);

    // computes the inverse of the matrix
    Mat4 Inverse(Mat4 const&);
    Quat Inverse(Quat const&);

    // right-hand multiply
    Mat4 Scale(Mat4 const&, Vec3 const&);
    Mat4 Rotate(Mat4 const&, float, Vec3 const&);
    Mat4 Translate(Mat4 const&, Vec3 const&);

    Quat QuatCast(Mat3 const&);
    Mat3 ToMat3(Quat const&);

    Vec3 EulerAngles(Quat const&);

    // returns `true` if the two arguments are effectively equal (i.e. within machine precision)
    //
    // this algorithm is designed to be correct, rather than fast
    bool IsEffectivelyEqual(double, double) noexcept;

    bool IsLessThanOrEffectivelyEqual(double, double) noexcept;

    // returns `true` if the first two arguments are within `relativeError` of eachover
    //
    // careful: this won't work if either argument is equal to, or very near, zero (relative
    //          requires scaling)
    bool IsEqualWithinRelativeError(double, double, double relativeError) noexcept;
    bool IsEqualWithinRelativeError(float, float, float relativeError) noexcept;
    bool IsEqualWithinRelativeError(Vec3 const&, Vec3 const&, float relativeError) noexcept;

    // returns `true` if the first two arguments are within `absoluteError` of eachover
    bool IsEqualWithinAbsoluteError(float, float, float absError) noexcept;
    bool IsEqualWithinAbsoluteError(Vec3 const&, Vec3 const&, float absError) noexcept;

    // ----- VecX/MatX helpers -----

    // returns true if the provided vectors are at the same location
    bool AreAtSameLocation(Vec3 const&, Vec3 const&) noexcept;

    // returns a vector containing min(a[dim], b[dim]) for each dimension
    Vec3 Min(Vec3 const&, Vec3 const&) noexcept;

    // returns a vector containing min(a[dim], b[dim]) for each dimension
    Vec2 Min(Vec2 const&, Vec2 const&) noexcept;

    // returns a vector containing min(a[dim], b[dim]) for each dimension
    Vec2i Min(Vec2i const&, Vec2i const&) noexcept;

    // returns a vector containing max(a[dim], b[dim]) for each dimension
    Vec3 Max(Vec3 const&, Vec3 const&) noexcept;

    // returns a vector containing max(a[dim], b[dim]) for each dimension
    Vec2 Max(Vec2 const&, Vec2 const&) noexcept;

    // returns a vector containing min(a[dim], b[dim]) for each dimension
    Vec2i Max(Vec2i const&, Vec2i const&) noexcept;

    // returns the *index* of a vector's longest dimension
    Vec3::length_type LongestDimIndex(Vec3 const&) noexcept;

    // returns the *index* of a vector's longest dimension
    Vec2::length_type LongestDimIndex(Vec2) noexcept;

    // returns the *index* of a vector's longest dimension
    Vec2i::length_type LongestDimIndex(Vec2i) noexcept;

    // returns the *value* of a vector's longest dimension
    float LongestDim(Vec3 const&) noexcept;

    // returns the *value* of a vector's longest dimension
    float LongestDim(Vec2) noexcept;

    // returns the *value* of a vector's longest dimension
    Vec2i::value_type LongestDim(Vec2i) noexcept;

    // returns the aspect ratio of the vec (effectively: x/y)
    float AspectRatio(Vec2i) noexcept;

    // returns the aspect ratio of the vec (effectively: x/y)
    float AspectRatio(Vec2) noexcept;

    // returns the midpoint between two vectors (effectively: 0.5 * (a+b))
    Vec2 Midpoint(Vec2, Vec2) noexcept;

    // returns the midpoint between two vectors (effectively: 0.5 * (a+b))
    Vec3 Midpoint(Vec3 const&, Vec3 const&) noexcept;

    // returns the unweighted midpoint of all of the provided vectors, or {0.0f, 0.0f, 0.0f} if provided no inputs
    Vec3 Midpoint(nonstd::span<Vec3 const>) noexcept;

    // returns the sum of `n` vectors using the "Kahan Summation Algorithm" to reduce errors, returns {0.0f, 0.0f, 0.0f} if provided no inputs
    Vec3 KahanSum(nonstd::span<Vec3 const>) noexcept;

    // returns the average of `n` vectors using whichever numerically stable average happens to work
    Vec3 NumericallyStableAverage(nonstd::span<Vec3 const>) noexcept;

    // returns a normal vector of the supplied (pointed to) triangle (i.e. (v[1]-v[0]) x (v[2]-v[0]))
    Vec3 TriangleNormal(Triangle const&) noexcept;

    // returns the adjugate matrix of the given 3x3 input
    Mat3 ToAdjugateMatrix(Mat3 const&) noexcept;

    // returns a normal matrix created from the supplied xform matrix
    Mat3 ToNormalMatrix(Mat4 const&) noexcept;

    // returns a normal matrix created from the supplied xform matrix
    Mat3 ToNormalMatrix(Mat4x3 const&) noexcept;

    // returns a noraml matrix created from the supplied xform matrix
    //
    // equivalent to mat4{ToNormalMatrix(m)}
    Mat4 ToNormalMatrix4(Mat4 const&) noexcept;

    // returns a transform matrix that rotates dir1 to point in the same direction as dir2
    Mat4 Dir1ToDir2Xform(Vec3 const& dir1, Vec3 const& dir2) noexcept;

    // returns euler angles for performing an intrinsic, step-by-step, rotation about X, Y, and then Z
    Vec3 ExtractEulerAngleXYZ(Quat const&) noexcept;

    // returns euler angles for performing an intrinsic, step-by-step, rotation about X, Y, and then Z
    Vec3 ExtractEulerAngleXYZ(Mat4 const&) noexcept;

    // returns an XY NDC point converted from a screen/viewport point
    //
    // - input screen point has origin in top-left, Y goes down
    // - input screen point has range: (0,0) is top-left, (1, 1) is bottom-right
    // - output NDC point has origin in middle, Y goes up
    // - output NDC point has range: (-1, 1) is top-left, (1, -1) is bottom-right
    Vec2 TopleftRelPosToNDCPoint(Vec2 relpos);

    // returns an XY top-left relative point converted from the given NDC point
    //
    // - input NDC point has origin in the middle, Y goes up
    // - input NDC point has range: (-1, -1) for top-left, (1, -1) is bottom-right
    // - output point has origin in the top-left, Y goes down
    // - output point has range: (0, 0) for top-left, (1, 1) for bottom-right
    Vec2 NDCPointToTopLeftRelPos(Vec2 ndcPos);

    // returns an NDC affine point vector (i.e. {x, y, z, 1.0}) converted from a screen/viewport point
    //
    // - input screen point has origin in top-left, Y goes down
    // - input screen point has range: (0,0) is top-left, (1, 1) is bottom-right
    // - output NDC point has origin in middle, Y goes up
    // - output NDC point has range -1 to +1 in each dimension
    // - output assumes Z is "at the front of the cube" (Z = -1.0f)
    // - output will therefore be: {xNDC, yNDC, -1.0f, 1.0f}
    Vec4 TopleftRelPosToNDCCube(Vec2 relpos);

    // "un-project" a screen/viewport point into 3D world-space, assuming a
    // perspective camera
    //
    // - input screen point has origin in top-left, Y goes down
    // - input screen point has range: (0,0) is top-left, (1, 1) is bottom-right
    // - `cameraWorldspaceOrigin` is the location of the camera in world space
    // - `cameraViewMatrix` transforms points from world-space to view-space
    // - `cameraProjMatrix` transforms points from view-space to world-space
    Line PerspectiveUnprojectTopLeftScreenPosToWorldRay(
        Vec2 relpos,
        Vec3 cameraWorldspaceOrigin,
        Mat4 const& cameraViewMatrix,
        Mat4 const& cameraProjMatrix
    );

    // ----- `Rect` helpers -----

    // returns `Min(rect.p1, rect.p2)`: i.e. the smallest X and the smallest Y of the rectangle's points
    Vec2 MinValuePerDimension(Rect const&) noexcept;

    // returns the area of the rectangle
    float Area(Rect const&) noexcept;

    // returns the dimensions of the rectangle
    Vec2 Dimensions(Rect const&) noexcept;

    // returns bottom-left point of the rectangle
    Vec2 BottomLeft(Rect const&) noexcept;

    // returns the aspect ratio (width/height) of the rectangle
    float AspectRatio(Rect const&) noexcept;

    // returns the middle point of the rectangle
    Vec2 Midpoint(Rect const&) noexcept;

    // returns the smallest rectangle that bounds the provided points
    //
    // note: no points --> zero-sized rectangle at the origin
    Rect BoundingRectOf(nonstd::span<Vec2 const>);

    // returns a rectangle that has been expanded along each edge by the given amount
    //
    // (e.g. expand 1.0f adds 1.0f to both the left edge and the right edge)
    Rect Expand(Rect const&, float) noexcept;
    Rect Expand(Rect const&, Vec2) noexcept;

    // returns a rectangle where both p1 and p2 are clamped between min and max (inclusive)
    Rect Clamp(Rect const&, Vec2 const& min, Vec2 const& max) noexcept;

    // returns a rect, created by mapping an Normalized Device Coordinates (NDC) rect
    // (i.e. -1.0 to 1.0) within a screenspace viewport (pixel units, topleft == (0, 0))
    Rect NdcRectToScreenspaceViewportRect(Rect const& ndcRect, Rect const& viewport) noexcept;


    // ----- `Sphere` helpers -----

    // returns a sphere that bounds the given vertices
    Sphere BoundingSphereOf(nonstd::span<Vec3 const>) noexcept;

    // returns a sphere that loosely bounds the given AABB
    Sphere ToSphere(AABB const&) noexcept;

    // returns an xform that maps an origin centered r=1 sphere into an in-scene sphere
    Mat4 FromUnitSphereMat4(Sphere const&) noexcept;

    // returns an xform that maps a sphere to another sphere
    Mat4 SphereToSphereMat4(Sphere const&, Sphere const&) noexcept;
    Transform SphereToSphereTransform(Sphere const&, Sphere const&) noexcept;

    // returns an AABB that contains the sphere
    AABB ToAABB(Sphere const&) noexcept;


    // ----- `Line` helpers -----

    // returns a line that has been transformed by the supplied transform matrix
    Line TransformLine(Line const&, Mat4 const&) noexcept;

    // returns a line that has been transformed by the inverse of the supplied transform
    Line InverseTransformLine(Line const&, Transform const&) noexcept;


    // ----- `Disc` helpers -----

    // returns an xform that maps a disc to another disc
    Mat4 DiscToDiscMat4(Disc const&, Disc const&) noexcept;


    // ----- `AABB` helpers -----

    // returns an AABB that is "inverted", such that it's minimum is the largest
    // possible value and its maximum is the smallest possible value
    //
    // handy as a "seed" when union-ing many AABBs, because it won't
    AABB InvertedAABB() noexcept;

    // returns the centerpoint of an AABB
    Vec3 Midpoint(AABB const&) noexcept;

    // returns the dimensions of an AABB
    Vec3 Dimensions(AABB const&) noexcept;

    // returns the volume of the AABB
    float Volume(AABB const&) noexcept;

    // returns the smallest AABB that spans both of the provided AABBs
    AABB Union(AABB const&, AABB const&) noexcept;

    // returns true if the AABB has no extents in any dimension
    bool IsAPoint(AABB const&) noexcept;

    // returns true if the AABB is an extent in any dimension is zero
    bool IsZeroVolume(AABB const&) noexcept;

    // returns the *index* of the longest dimension of an AABB
    Vec3::length_type LongestDimIndex(AABB const&) noexcept;

    // returns the length of the longest dimension of an AABB
    float LongestDim(AABB const&) noexcept;

    // returns the eight corner points of the cuboid representation of the AABB
    std::array<Vec3, 8> ToCubeVerts(AABB const&) noexcept;

    // returns an AABB that has been transformed by the given matrix
    AABB TransformAABB(AABB const&, Mat4 const&) noexcept;

    // returns an AABB that has been transformed by the given transform
    AABB TransformAABB(AABB const&, Transform const&) noexcept;

    // returns an AAB that tightly bounds the provided triangle
    AABB AABBFromTriangle(Triangle const& t) noexcept;

    // returns an AABB that tightly bounds the provided points
    AABB AABBFromVerts(nonstd::span<Vec3 const>) noexcept;

    // returns an AABB that tightly bounds the points indexed by the provided 32-bit indices
    AABB AABBFromIndexedVerts(nonstd::span<Vec3 const> verts, nonstd::span<uint32_t const> indices);

    // returns an AABB that tightly bounds the points indexed by the provided 16-bit indices
    AABB AABBFromIndexedVerts(nonstd::span<Vec3 const> verts, nonstd::span<uint16_t const> indices);

    // (tries to) return a Normalized Device Coordinate (NDC) rectangle, clamped to the NDC clipping
    // bounds ((-1,-1) to (1, 1)), where the rectangle loosely bounds the given worldspace AABB after
    // projecting it into NDC
    std::optional<Rect> AABBToScreenNDCRect(
        AABB const&,
        Mat4 const& viewMat,
        Mat4 const& projMat,
        float znear,
        float zfar
    );


    // ----- `Segment` helpers -----

    // returns a transform matrix that maps a path segment to another path segment
    Mat4 SegmentToSegmentMat4(Segment const&, Segment const&) noexcept;

    // returns a transform that maps a path segment to another path segment
    Transform SegmentToSegmentTransform(Segment const&, Segment const&) noexcept;

    // returns a transform that maps a Y-to-Y (bottom-to-top) cylinder to a segment with the given radius
    Transform YToYCylinderToSegmentTransform(Segment const&, float radius) noexcept;

    // returns a transform that maps a Y-to-Y (bottom-to-top) cone to a segment with the given radius
    Transform YToYConeToSegmentTransform(Segment const&, float radius) noexcept;


    // ----- `Transform` helpers -----

    // returns a 3x3 transform matrix equivalent to the provided transform (ignores position)
    Mat3 ToMat3(Transform const&) noexcept;

    // returns a 4x4 transform matrix equivalent to the provided transform
    Mat4 ToMat4(Transform const&) noexcept;

    // returns a 4x4 transform matrix equivalent to the inverse of the provided transform
    Mat4 ToInverseMat4(Transform const&) noexcept;

    // returns a 3x3 normal matrix for the provided transform
    Mat3 ToNormalMatrix(Transform const&) noexcept;

    // returns a 4x4 normal matrix for the provided transform
    Mat4 ToNormalMatrix4(Transform const&) noexcept;

    // returns a transform that *tries to* perform the equivalent transform as the provided mat4
    //
    // - not all 4x4 matrices can be expressed as an `Transform` (e.g. those containing skews)
    // - uses matrix decomposition to break up the provided matrix
    // - throws if decomposition of the provided matrix is not possible
    Transform ToTransform(Mat4 const&);

    // returns a unit-length vector that is the equivalent of the provided direction vector after applying the transform
    //
    // effectively, apply the Transform but ignore the `position` (translation) component
    Vec3 TransformDirection(Transform const&, Vec3 const&) noexcept;

    // returns a unit-length vector that is the equivalent of the provided direction vector after applying the inverse of the transform
    //
    // effectively, apply the inverse transform but ignore the `position` (translation) component
    Vec3 InverseTransformDirection(Transform const&, Vec3 const&) noexcept;

    // returns a vector that is the equivalent of the provided vector after applying the transform
    Vec3 TransformPoint(Transform const&, Vec3 const&) noexcept;

    // returns a vector that is the equivalent of the provided vector after applying the inverse of the transform
    Vec3 InverseTransformPoint(Transform const&, Vec3 const&) noexcept;

    // applies a world-space rotation to the transform
    void ApplyWorldspaceRotation(
        Transform& applicationTarget,
        Vec3 const& eulerAngles,
        Vec3 const& rotationCenter
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
    Vec3 ExtractEulerAngleXYZ(Transform const&) noexcept;

    // returns XYZ (pitch, yaw, roll) Euler angles for an extrinsic rotation
    //
    // in extrinsic rotations, each rotation happens about a *fixed* coordinate system, which
    // is in contrast to intrinsic rotations, which happen in a coordinate system that's attached
    // to a moving body (the thing being rotated)
    //
    // see: https://en.wikipedia.org/wiki/Euler_angles#Conventions_by_extrinsic_rotations
    Vec3 ExtractExtrinsicEulerAnglesXYZ(Transform const&) noexcept;
}
