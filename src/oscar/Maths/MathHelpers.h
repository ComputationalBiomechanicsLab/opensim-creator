#pragma once

#include <oscar/Maths/AABB.h>
#include <oscar/Maths/AABBFunctions.h>
#include <oscar/Maths/Angle.h>
#include <oscar/Maths/Eulers.h>
#include <oscar/Maths/Line.h>
#include <oscar/Maths/Mat3.h>
#include <oscar/Maths/Mat4.h>
#include <oscar/Maths/Quat.h>
#include <oscar/Maths/Sphere.h>
#include <oscar/Maths/Transform.h>
#include <oscar/Maths/Triangle.h>
#include <oscar/Maths/Vec.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/Maths/Vec4.h>
#include <oscar/Maths/TransformFunctions.h>

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <concepts>
#include <initializer_list>
#include <optional>
#include <span>
#include <type_traits>

namespace osc { struct Circle; }
namespace osc { struct Disc; }
namespace osc { struct Plane; }
namespace osc { struct Rect; }
namespace osc { struct LineSegment; }
namespace osc { struct Tetrahedron; }

// math helpers: generally handy math functions that aren't attached to a particular
//               osc struct
namespace osc
{
    // computes horizontal FoV for a given vertical FoV + aspect ratio
    Radians VerticalToHorizontalFOV(Radians vertical_fov, float aspectRatio);

    // ----- VecX/MatX helpers -----

    // returns the aspect ratio of the vec (effectively: x/y)
    float AspectRatio(Vec2i);

    // returns the aspect ratio of the vec (effectively: x/y)
    float AspectRatio(Vec2);

    // returns a transform matrix that rotates dir1 to point in the same direction as dir2
    Mat4 Dir1ToDir2Xform(Vec3 const& dir1, Vec3 const& dir2);

    // returns euler angles for performing an intrinsic, step-by-step, rotation about X, Y, and then Z
    Eulers extract_eulers_xyz(Quat const&);

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

    template<typename T>
    constexpr T Area(Vec<2, T> const& v) requires std::is_arithmetic_v<T>
    {
        return v.x * v.y;
    }

    // returns the area of the rectangle
    float Area(Rect const&);

    // returns the dimensions of the rectangle
    Vec2 dimensions(Rect const&);

    // returns bottom-left point of the rectangle
    Vec2 BottomLeft(Rect const&);

    // returns the aspect ratio (width/height) of the rectangle
    float AspectRatio(Rect const&);

    // returns the middle point of the rectangle
    Vec2 centroid(Rect const&);

    // returns the smallest rectangle that bounds the provided points
    //
    // note: no points --> zero-sized rectangle at the origin
    Rect BoundingRectOf(std::span<Vec2 const>);

    // returns the smallest rectangle that bounds the provided circle
    Rect BoundingRectOf(Circle const&);

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


    // ----- `Segment` helpers -----

    // returns a transform matrix that maps a path segment to another path segment
    Mat4 SegmentToSegmentMat4(LineSegment const&, LineSegment const&);

    // returns a transform that maps a path segment to another path segment
    Transform SegmentToSegmentTransform(LineSegment const&, LineSegment const&);

    // returns a transform that maps a Y-to-Y (bottom-to-top) cylinder to a segment with the given radius
    Transform YToYCylinderToSegmentTransform(LineSegment const&, float radius);

    // returns a transform that maps a Y-to-Y (bottom-to-top) cone to a segment with the given radius
    Transform YToYConeToSegmentTransform(LineSegment const&, float radius);

    // ----- other -----

    Vec3 transform_point(Mat4 const&, Vec3 const&);

    // returns the a quaternion equivalent to the given euler angles
    Quat WorldspaceRotation(Eulers const&);

    // applies a world-space rotation to the transform
    void ApplyWorldspaceRotation(
        Transform& applicationTarget,
        Eulers const& eulerAngles,
        Vec3 const& rotationCenter);

    float volume(Tetrahedron const&);

    // returns arrays that transforms cube faces from worldspace to projection
    // space such that the observer is looking at each face of the cube from
    // the center of the cube
    std::array<Mat4, 6> CalcCubemapViewProjMatrices(
        Mat4 const& projectionMatrix,
        Vec3 cubeCenter
    );
}
