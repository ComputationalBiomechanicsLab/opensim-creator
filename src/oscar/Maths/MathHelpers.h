#pragma once

#include <oscar/Maths/AABB.h>
#include <oscar/Maths/Angle.h>
#include <oscar/Maths/Eulers.h>
#include <oscar/Maths/LengthType.h>
#include <oscar/Maths/Line.h>
#include <oscar/Maths/Mat3.h>
#include <oscar/Maths/Mat4.h>
#include <oscar/Maths/Mat4x3.h>
#include <oscar/Maths/Qualifier.h>
#include <oscar/Maths/Quat.h>
#include <oscar/Maths/Sphere.h>
#include <oscar/Maths/Transform.h>
#include <oscar/Maths/Triangle.h>
#include <oscar/Maths/Vec.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/Maths/Vec4.h>

#include <glm/glm.hpp>

#include <array>
#include <cmath>
#include <cstdint>
#include <optional>
#include <span>
#include <type_traits>

namespace osc { struct Disc; }
namespace osc { struct Plane; }
namespace osc { struct Rect; }
namespace osc { struct Segment; }

// math helpers: generally handy math functions that aren't attached to a particular
//               osc struct
namespace osc
{
    template<std::floating_point Rep, AngularUnitTraits Units>
    Rep sin(Angle<Rep, Units> v)
    {
        return std::sin(RadiansT<Rep>{v}.count());
    }

    template<std::floating_point Rep, AngularUnitTraits Units>
    Rep cos(Angle<Rep, Units> v)
    {
        return std::cos(RadiansT<Rep>{v}.count());
    }

    template<std::floating_point Rep, AngularUnitTraits Units>
    Rep tan(Angle<Rep, Units> v)
    {
        return std::tan(RadiansT<Rep>{v}.count());
    }

    template<std::floating_point Rep>
    RadiansT<Rep> atan(Rep v)
    {
        return RadiansT<Rep>{std::atan(v)};
    }

    template<std::floating_point Rep>
    RadiansT<Rep> acos(Rep num)
    {
        return RadiansT<Rep>{std::acos(num)};
    }

    template<std::floating_point Rep>
    RadiansT<Rep> asin(Rep num)
    {
        return RadiansT<Rep>{std::asin(num)};
    }

    template<std::floating_point Rep>
    RadiansT<Rep> atan2(Rep x, Rep y)
    {
        return RadiansT<Rep>{std::atan2(x, y)};
    }

    template<
        std::floating_point Rep1,
        AngularUnitTraits Units1,
        std::floating_point Rep2,
        AngularUnitTraits Units2
    >
    typename std::common_type_t<Angle<Rep1, Units1>, Angle<Rep2, Units2>> fmod(Angle<Rep1, Units1> x, Angle<Rep2, Units2> y)
    {
        using CA = std::common_type_t<Angle<Rep1, Units1>, Angle<Rep2, Units2>>;
        return CA{std::fmod(CA{x}.count(), CA{y}.count())};
    }

    // returns the dot product of the provided two vectors
    template<LengthType L, typename T, Qualifier Q>
    constexpr T Dot(Vec<L, T, Q> const& a, Vec<L, T, Q> const& b)
        requires std::is_arithmetic_v<T>
    {
        return glm::dot(a, b);
    }

    // returns the smallest of `a` and `b`
    template<typename GenType>
    constexpr GenType Min(GenType a, GenType b)
        requires std::is_arithmetic_v<GenType>
    {
        return glm::min(a, b);
    }

    // returns a vector containing min(a[dim], b[dim]) for each element
    template<LengthType L, typename T, Qualifier Q>
    constexpr Vec<L, T, Q> Min(Vec<L, T, Q> const& a, Vec<L, T, Q> const& b)
        requires std::is_arithmetic_v<T>
    {
        return glm::min(a, b);
    }

    // returns the largest of `a` and `b`
    template<typename GenType>
    constexpr GenType Max(GenType a, GenType b)
        requires std::is_arithmetic_v<GenType>
    {
        return glm::max(a, b);
    }

    // returns a vector containing max(a[i], b[i]) for each element
    template<LengthType L, typename T, Qualifier Q>
    constexpr Vec<L, T, Q> Max(Vec<L, T, Q> const& a, Vec<L, T, Q> const& b)
        requires std::is_arithmetic_v<T>
    {
        return glm::max(a, b);
    }

    // clamps `v` between `low` and `high` (inclusive)
    template<typename GenType>
    constexpr GenType Clamp(GenType v, GenType low, GenType high)
        requires std::is_arithmetic_v<GenType>
    {
        return glm::clamp(v, low, high);
    }

    template<std::floating_point Rep, AngularUnitTraits Units>
    constexpr Angle<Rep, Units> Clamp(Angle<Rep, Units> const& v, Angle<Rep, Units> const& min, Angle<Rep, Units> const& max)
    {
        return Angle<Rep, Units>{Clamp(v.count(), min.count(), max.count())};
    }

    // clamps each element in `x` between the corresponding elements in `minVal` and `maxVal`
    template<LengthType L, typename T, Qualifier Q>
    constexpr Vec<L, T, Q> Clamp(Vec<L, T, Q> const& x, Vec<L, T, Q> const& minVal, Vec<L, T, Q> const& maxVal)
        requires std::is_arithmetic_v<T>
    {
        return glm::clamp(x, minVal, maxVal);
    }

    // clamps each element in `x` between `minVal` and `maxVal`
    template<LengthType L, typename T, Qualifier Q>
    constexpr Vec<L, T, Q> Clamp(Vec<L, T, Q> const& x, T const& minVal, T const& maxVal)
        requires std::is_arithmetic_v<T>
    {
        return glm::clamp(x, minVal, maxVal);
    }

    // linearly interpolates between `x` and `y` with factor `a`
    template<typename GenType, typename UInterpolant>
    constexpr GenType Mix(GenType const& x, GenType const& y, UInterpolant const& a)
        requires std::is_arithmetic_v<GenType> && std::is_floating_point_v<UInterpolant>
    {
        return glm::mix(x, y, a);
    }

    // linearly interpolates between each element in `x` and `y` with factor `a`
    template<LengthType L, typename T, Qualifier Q, typename UInterpolant>
    constexpr Vec<L, T, Q> Mix(Vec<L, T, Q> const& x, Vec<L, T, Q> const& y, UInterpolant const& a)
        requires std::is_arithmetic_v<T> && std::is_floating_point_v<UInterpolant>
    {
        return glm::mix(x, y, a);
    }

    // calculates the cross product of the two vectors
    template<typename T, Qualifier Q>
    constexpr Vec<3, T, Q> Cross(Vec<3, T, Q> const& x, Vec<3, T, Q> const& y)
        requires std::is_floating_point_v<T>
    {
        return glm::cross(x, y);
    }

    // returns a normalized version of the provided argument
    template<typename T, Qualifier Q>
    Qua<T, Q> Normalize(Qua<T, Q> const& q)
        requires std::is_floating_point_v<T>
    {
        return glm::normalize(q);
    }

    template<LengthType L, typename T, Qualifier Q>
    Vec<L, T, Q> Normalize(Vec<L, T, Q> const& v)
        requires std::is_floating_point_v<T>
    {
        return glm::normalize(v);
    }

    // returns the length of the provided vector
    template<LengthType L, typename T, Qualifier Q>
    float Length(Vec<L, T, Q> const& v)
        requires std::is_floating_point_v<T>
    {
        return glm::length(v);
    }

    // returns the squared length of the provided vector
    template<LengthType L, typename T, Qualifier Q>
    float Length2(Vec<L, T, Q> const& v)
        requires std::is_floating_point_v<T>
    {
        return glm::length2(v);
    }

    // computes the rotation from `src` to `dest`
    Quat Rotation(Vec3 const& src, Vec3 const& dest);

    // computes a rotation from an angle+axis
    Quat AngleAxis(Radians angle, Vec3 const& axis);

    // computes a view matrix for the given params
    Mat4 LookAt(Vec3 const& eye, Vec3 const& center, Vec3 const& up);

    // computes horizontal FoV for a given vertical FoV + aspect ratio
    Radians VerticalToHorizontalFOV(Radians verticalFOV, float aspectRatio);

    // computes a perspective projection matrix
    Mat4 Perspective(Radians verticalFOV, float aspectRatio, float zNear, float zFar);

    // computes an orthogonal projection matrix
    Mat4 Ortho(float left, float right, float bottom, float top, float zNear, float zFar);

    // computes the inverse of the matrix
    Mat4 Inverse(Mat4 const&);
    Quat Inverse(Quat const&);

    // right-hand multiply
    Mat4 Scale(Mat4 const&, Vec3 const&);
    Mat4 Rotate(Mat4 const&, Radians angle, Vec3 const& axis);
    Mat4 Translate(Mat4 const&, Vec3 const&);

    Quat QuatCast(Mat3 const&);
    Mat3 ToMat3(Quat const&);

    Eulers EulerAngles(Quat const&);

    // returns `true` if the two arguments are effectively equal (i.e. within machine precision)
    //
    // this algorithm is designed to be correct, rather than fast
    bool IsEffectivelyEqual(double, double);

    bool IsLessThanOrEffectivelyEqual(double, double);

    // returns `true` if the first two arguments are within `relativeError` of eachover
    //
    // careful: this won't work if either argument is equal to, or very near, zero (relative
    //          requires scaling)
    bool IsEqualWithinRelativeError(double, double, double relativeError);
    bool IsEqualWithinRelativeError(float, float, float relativeError);

    template<LengthType L, typename T, Qualifier Q>
    bool IsEqualWithinRelativeError(Vec<L, T, Q> const& a, Vec<L, T, Q> const& b, T relativeError)
        requires std::is_floating_point_v<T>
    {
        for (LengthType i = 0; i < L; ++i)
        {
            if (!IsEqualWithinRelativeError(a[i], b[i], relativeError))
            {
                return false;
            }
        }
        return true;
    }

    // returns `true` if the first two arguments are within `absoluteError` of eachover
    bool IsEqualWithinAbsoluteError(float, float, float absError);

    template<LengthType L, typename T, Qualifier Q>
    bool IsEqualWithinAbsoluteError(Vec<L, T, Q> const& a, Vec<L, T, Q> const& b, T absError)
        requires std::is_floating_point_v<T>
    {
        for (LengthType i = 0; i < L; ++i)
        {
            if (!IsEqualWithinAbsoluteError(a[i], b[i], absError))
            {
                return false;
            }
        }
        return true;
    }

    // ----- VecX/MatX helpers -----

    // returns true if the provided vectors are at the same location
    template<LengthType L, typename T, Qualifier Q>
    bool AreAtSameLocation(Vec<L, T, Q> const& a, Vec<L, T, Q> const& b)
        requires std::is_arithmetic_v<T>
    {
        constexpr T eps2 = std::numeric_limits<T>::epsilon() * std::numeric_limits<T>::epsilon();
        auto const b2a = a - b;
        return Dot(b2a, b2a) > eps2;
    }

    bool AreAtSameLocation(Vec3 const&, Vec3 const&);

    // returns the *index* of a vector's longest dimension
    Vec3::length_type LongestDimIndex(Vec3 const&);

    // returns the *index* of a vector's longest dimension
    Vec2::length_type LongestDimIndex(Vec2);

    // returns the *index* of a vector's longest dimension
    Vec2i::length_type LongestDimIndex(Vec2i);

    // returns the *value* of a vector's longest dimension
    float LongestDim(Vec3 const&);

    // returns the *value* of a vector's longest dimension
    float LongestDim(Vec2);

    // returns the *value* of a vector's longest dimension
    Vec2i::value_type LongestDim(Vec2i);

    // returns the aspect ratio of the vec (effectively: x/y)
    float AspectRatio(Vec2i);

    // returns the aspect ratio of the vec (effectively: x/y)
    float AspectRatio(Vec2);

    // returns the midpoint between two vectors (effectively: 0.5 * (a+b))
    Vec2 Midpoint(Vec2, Vec2);

    // returns the midpoint between two vectors (effectively: 0.5 * (a+b))
    Vec3 Midpoint(Vec3 const&, Vec3 const&);

    // returns the unweighted midpoint of all of the provided vectors, or {0.0f, 0.0f, 0.0f} if provided no inputs
    Vec3 Midpoint(std::span<Vec3 const>);

    // returns the sum of `n` vectors using the "Kahan Summation Algorithm" to reduce errors, returns {0.0f, 0.0f, 0.0f} if provided no inputs
    Vec3 KahanSum(std::span<Vec3 const>);

    // returns the average of `n` vectors using whichever numerically stable average happens to work
    Vec3 NumericallyStableAverage(std::span<Vec3 const>);

    // returns a normal vector of the supplied (pointed to) triangle (i.e. (v[1]-v[0]) x (v[2]-v[0]))
    Vec3 TriangleNormal(Triangle const&);

    // returns the adjugate matrix of the given 3x3 input
    Mat3 ToAdjugateMatrix(Mat3 const&);

    // returns a normal matrix created from the supplied xform matrix
    Mat3 ToNormalMatrix(Mat4 const&);

    // returns a normal matrix created from the supplied xform matrix
    Mat3 ToNormalMatrix(Mat4x3 const&);

    // returns a noraml matrix created from the supplied xform matrix
    //
    // equivalent to mat4{ToNormalMatrix(m)}
    Mat4 ToNormalMatrix4(Mat4 const&);

    // returns a transform matrix that rotates dir1 to point in the same direction as dir2
    Mat4 Dir1ToDir2Xform(Vec3 const& dir1, Vec3 const& dir2);

    // returns euler angles for performing an intrinsic, step-by-step, rotation about X, Y, and then Z
    Eulers ExtractEulerAngleXYZ(Quat const&);

    // returns euler angles for performing an intrinsic, step-by-step, rotation about X, Y, and then Z
    Eulers ExtractEulerAngleXYZ(Mat4 const&);

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
    Vec2 MinValuePerDimension(Rect const&);

    template<typename T, Qualifier Q>
    constexpr T Area(Vec<2, T, Q> const& v)
        requires std::is_arithmetic_v<T>
    {
        return v.x * v.y;
    }

    // returns the area of the rectangle
    float Area(Rect const&);

    // returns the dimensions of the rectangle
    Vec2 Dimensions(Rect const&);

    // returns bottom-left point of the rectangle
    Vec2 BottomLeft(Rect const&);

    // returns the aspect ratio (width/height) of the rectangle
    float AspectRatio(Rect const&);

    // returns the middle point of the rectangle
    Vec2 Midpoint(Rect const&);

    // returns the smallest rectangle that bounds the provided points
    //
    // note: no points --> zero-sized rectangle at the origin
    Rect BoundingRectOf(std::span<Vec2 const>);

    // returns a rectangle that has been expanded along each edge by the given amount
    //
    // (e.g. expand 1.0f adds 1.0f to both the left edge and the right edge)
    Rect Expand(Rect const&, float);
    Rect Expand(Rect const&, Vec2);

    // returns a rectangle where both p1 and p2 are clamped between min and max (inclusive)
    Rect Clamp(Rect const&, Vec2 const& min, Vec2 const& max);

    // returns a rect, created by mapping an Normalized Device Coordinates (NDC) rect
    // (i.e. -1.0 to 1.0) within a screenspace viewport (pixel units, topleft == (0, 0))
    Rect NdcRectToScreenspaceViewportRect(Rect const& ndcRect, Rect const& viewport);


    // ----- `Sphere` helpers -----

    // returns a sphere that bounds the given vertices
    Sphere BoundingSphereOf(std::span<Vec3 const>);

    // returns a sphere that loosely bounds the given AABB
    Sphere ToSphere(AABB const&);

    // returns an xform that maps an origin centered r=1 sphere into an in-scene sphere
    Mat4 FromUnitSphereMat4(Sphere const&);

    // returns an xform that maps a sphere to another sphere
    Mat4 SphereToSphereMat4(Sphere const&, Sphere const&);
    Transform SphereToSphereTransform(Sphere const&, Sphere const&);

    // returns an AABB that contains the sphere
    AABB ToAABB(Sphere const&);


    // ----- `Line` helpers -----

    // returns a line that has been transformed by the supplied transform matrix
    Line TransformLine(Line const&, Mat4 const&);

    // returns a line that has been transformed by the inverse of the supplied transform
    Line InverseTransformLine(Line const&, Transform const&);


    // ----- `Disc` helpers -----

    // returns an xform that maps a disc to another disc
    Mat4 DiscToDiscMat4(Disc const&, Disc const&);


    // ----- `AABB` helpers -----

    // returns an AABB that is "inverted", such that it's minimum is the largest
    // possible value and its maximum is the smallest possible value
    //
    // handy as a "seed" when union-ing many AABBs, because it won't
    AABB InvertedAABB();

    // returns the centerpoint of an AABB
    Vec3 Midpoint(AABB const&);

    // returns the dimensions of an AABB
    Vec3 Dimensions(AABB const&);

    // returns the half-widths of an AABB (effectively, Dimensions(aabb)/2.0)
    Vec3 HalfWidths(AABB const&);

    // returns the volume of the AABB
    float Volume(AABB const&);

    // returns the smallest AABB that spans both of the provided AABBs
    AABB Union(AABB const&, AABB const&);

    // returns true if the AABB has no extents in any dimension
    bool IsAPoint(AABB const&);

    // returns true if the AABB is an extent in any dimension is zero
    bool IsZeroVolume(AABB const&);

    // returns the *index* of the longest dimension of an AABB
    Vec3::length_type LongestDimIndex(AABB const&);

    // returns the length of the longest dimension of an AABB
    float LongestDim(AABB const&);

    // returns the eight corner points of the cuboid representation of the AABB
    std::array<Vec3, 8> ToCubeVerts(AABB const&);

    // returns an AABB that has been transformed by the given matrix
    AABB TransformAABB(AABB const&, Mat4 const&);

    // returns an AABB that has been transformed by the given transform
    AABB TransformAABB(AABB const&, Transform const&);

    // returns an AAB that tightly bounds the provided triangle
    AABB AABBFromTriangle(Triangle const&);

    // returns an AABB that tightly bounds the provided points
    AABB AABBFromVerts(std::span<Vec3 const>);

    // returns an AABB that tightly bounds the provided points (alias that matches BoundingSphereOf, etc.)
    inline AABB BoundingAABBOf(std::span<Vec3 const> vs) { return AABBFromVerts(vs); }

    // returns an AABB that tightly bounds the points indexed by the provided 32-bit indices
    AABB AABBFromIndexedVerts(std::span<Vec3 const> verts, std::span<uint32_t const> indices);

    // returns an AABB that tightly bounds the points indexed by the provided 16-bit indices
    AABB AABBFromIndexedVerts(std::span<Vec3 const> verts, std::span<uint16_t const> indices);

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
    Mat4 SegmentToSegmentMat4(Segment const&, Segment const&);

    // returns a transform that maps a path segment to another path segment
    Transform SegmentToSegmentTransform(Segment const&, Segment const&);

    // returns a transform that maps a Y-to-Y (bottom-to-top) cylinder to a segment with the given radius
    Transform YToYCylinderToSegmentTransform(Segment const&, float radius);

    // returns a transform that maps a Y-to-Y (bottom-to-top) cone to a segment with the given radius
    Transform YToYConeToSegmentTransform(Segment const&, float radius);


    // ----- `Transform` helpers -----

    // returns a 3x3 transform matrix equivalent to the provided transform (ignores position)
    Mat3 ToMat3(Transform const&);

    // returns a 4x4 transform matrix equivalent to the provided transform
    Mat4 ToMat4(Transform const&);

    // returns a 4x4 transform matrix equivalent to the inverse of the provided transform
    Mat4 ToInverseMat4(Transform const&);

    // returns a 3x3 normal matrix for the provided transform
    Mat3 ToNormalMatrix(Transform const&);

    // returns a 4x4 normal matrix for the provided transform
    Mat4 ToNormalMatrix4(Transform const&);

    // returns a transform that *tries to* perform the equivalent transform as the provided mat4
    //
    // - not all 4x4 matrices can be expressed as an `Transform` (e.g. those containing skews)
    // - uses matrix decomposition to break up the provided matrix
    // - throws if decomposition of the provided matrix is not possible
    Transform ToTransform(Mat4 const&);

    // returns a unit-length vector that is the equivalent of the provided direction vector after applying the transform
    //
    // effectively, apply the Transform but ignore the `position` (translation) component
    Vec3 TransformDirection(Transform const&, Vec3 const&);

    // returns a unit-length vector that is the equivalent of the provided direction vector after applying the inverse of the transform
    //
    // effectively, apply the inverse transform but ignore the `position` (translation) component
    Vec3 InverseTransformDirection(Transform const&, Vec3 const&);

    // returns a vector that is the equivalent of the provided vector after applying the transform
    Vec3 TransformPoint(Transform const&, Vec3 const&);
    Vec3 TransformPoint(Mat4 const&, Vec3 const&);

    // returns a vector that is the equivalent of the provided vector after applying the inverse of the transform
    Vec3 InverseTransformPoint(Transform const&, Vec3 const&);

    // returns the a quaternion equivalent to the given euler angles
    Quat WorldspaceRotation(Eulers const&);

    // applies a world-space rotation to the transform
    void ApplyWorldspaceRotation(
        Transform& applicationTarget,
        Eulers const& eulerAngles,
        Vec3 const& rotationCenter
    );

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
    Eulers ExtractEulerAngleXYZ(Transform const&);

    // returns XYZ (pitch, yaw, roll) Euler angles for an extrinsic rotation
    //
    // in extrinsic rotations, each rotation happens about a *fixed* coordinate system, which
    // is in contrast to intrinsic rotations, which happen in a coordinate system that's attached
    // to a moving body (the thing being rotated)
    //
    // see: https://en.wikipedia.org/wiki/Euler_angles#Conventions_by_extrinsic_rotations
    Eulers ExtractExtrinsicEulerAnglesXYZ(Transform const&);

    // returns the provided transform, but rotated such that the given axis, as expressed
    // in the original transform, will instead point along the new direction
    Transform PointAxisAlong(Transform const&, int axisIndex, Vec3 const& newDirection);

    // returns the provided transform, but rotated such that the given axis, as expressed
    // in the original transform, will instead point towards the given point
    //
    // alternate explanation: "performs the shortest (angular) rotation of the given
    // transform such that the given axis points towards a point in the same space"
    Transform PointAxisTowards(Transform const&, int axisIndex, Vec3 const& location);

    // returns the provided transform, but intrinsically rotated along the given axis by
    // the given number of radians
    Transform RotateAlongAxis(Transform const&, int axisIndex, Radians);
}
