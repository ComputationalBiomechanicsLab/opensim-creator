#pragma once

#include <liboscar/Maths/AABB.h>
#include <liboscar/Maths/Angle.h>
#include <liboscar/Maths/EulerAngles.h>
#include <liboscar/Maths/Line.h>
#include <liboscar/Maths/Mat3.h>
#include <liboscar/Maths/Mat4.h>
#include <liboscar/Maths/Quat.h>
#include <liboscar/Maths/Rect.h>
#include <liboscar/Maths/Sphere.h>
#include <liboscar/Maths/Transform.h>
#include <liboscar/Maths/Vec.h>
#include <liboscar/Maths/Vec2.h>
#include <liboscar/Maths/Vec3.h>
#include <liboscar/Maths/Vec4.h>
#include <liboscar/Maths/TransformFunctions.h>

#include <array>
#include <span>

namespace osc { struct Circle; }
namespace osc { struct Disc; }
namespace osc { struct Plane; }
namespace osc { struct LineSegment; }
namespace osc { struct Tetrahedron; }

// math helpers: generally handy math functions that aren't attached to a particular
//               osc struct
namespace osc
{
    // computes horizontal FoV for a given vertical FoV + aspect ratio
    Radians vertical_to_horizontal_field_of_view(Radians vertical_field_of_view, float aspect_ratio);

    // returns a native-device-coordinate-like (NDC-like) point converted from a point
    // defined in a normalized y-points-down space.
    //
    // - input point should have origin in top-left, Y goes down
    // - input point should have normalized range: (0, 0) is top-left, (+1, +1) is bottom-right
    // - output point's origin will be in the middle of the input space, Y goes up
    // - output point's range will be origin-centered: (-1, -1) is bottom-left, (+1, +1) is top-right
    Vec2 topleft_normalized_point_to_ndc(Vec2 normalized_point);

    // returns a normalized y-points-down point converted from a point defined in a
    // native-device-coordinate-like (NDC-like) space.
    //
    // - input point should have origin in the middle, Y goes up
    // - input point should have an origin-centered range: (-1, -1) is bottom-left, (+1, +1) is top-right
    // - output point origin will have an origin in the top-left, Y goes down
    // - output point has range: (0, 0) for top-left, (1, 1) for bottom-right
    Vec2 ndc_point_to_topleft_normalized(Vec2 ndc_point);

    // returns an NDC affine point (i.e. {x, y, z, 1.0}) converted from a point
    // defined in a normalized y-points-down space.
    //
    // - input point should have origin in top-left, Y goes down
    // - input point should have normalized range: (0, 0) is top-left, (+1, +1) is bottom-right
    // - output point origin will have an origin in the top-left, Y goes down
    // - output point has range: (0, 0) for top-left, (1, 1) for bottom-right
    // - output point will have a `z` of `-1.0f` (i.e. nearest depth)
    Vec4 topleft_normalized_point_to_ndc_cube(Vec2 normalized_point);

    // "un-project" a point defined in a normalized y-points-down space into world
    // space, assuming a perspective projection
    //
    // - input point should have origin in top-left, Y goes down
    // - input point should have normalized range: (0, 0) is top-left, (+1, +1) is bottom-right
    // - `camera_world_space_origin` is the location of the camera in world space
    // - `camera_view_matrix` transforms points from world space to view space
    // - `camera_proj_matrix` transforms points from view space to world space
    Line perspective_unproject_topleft_normalized_pos_to_world(
        Vec2 normalized_point,
        Vec3 camera_world_space_origin,
        const Mat4& camera_view_matrix,
        const Mat4& camera_proj_matrix
    );

    // returns a rectangular subspace defined in, and bounded by, native device
    // coordinates (NDC) to a subspace defined in a normalized y-points-down space
    // bounded by `viewport`.
    Rect ndc_rect_to_topleft_viewport_rect(const Rect& ndc_rect, const Rect& viewport);

    // returns the location where `world_space_location` would occur when projected via the
    // given `view_matrix` and `projection_matrix`es onto `viewport_rect`.
    Vec2 project_onto_viewport_rect(
        const Vec3& world_space_location,
        const Mat4& view_matrix,
        const Mat4& projection_matrix,
        const Rect& viewport_rect
    );


    // ----- `Sphere` helpers -----

    // returns a `Sphere` that loosely bounds the given `Vec3`s
    Sphere bounding_sphere_of(std::span<const Vec3>);

    // returns a `Sphere` that loosely bounds the given `AABB`
    Sphere bounding_sphere_of(const AABB&);

    // returns an `AABB` that tightly bounds the `Sphere`
    AABB bounding_aabb_of(const Sphere&);


    // ----- `Line` helpers -----

    // returns a `Line` that has been transformed by the `Mat4`
    Line transform_line(const Line&, const Mat4&);

    // returns a `Line` that has been transformed by the inverse of the supplied `Transform`
    Line inverse_transform_line(const Line&, const Transform&);


    // ----- `Disc` helpers -----

    // returns a `Mat4` that maps one `Disc` to another `Disc`
    Mat4 mat4_transform_between(const Disc&, const Disc&);


    // ----- `Segment` helpers -----

    // returns a transform matrix that maps a path segment to another path segment
    Mat4 mat4_transform_between(const LineSegment&, const LineSegment&);

    // returns a `Transform` that maps a path segment to another path segment
    Transform transform_between(const LineSegment&, const LineSegment&);

    // returns a `Transform` that maps a Y-to-Y (bottom-to-top) cylinder to a segment with the given radius
    Transform cylinder_to_line_segment_transform(const LineSegment&, float radius);

    // returns a `Transform` that maps a Y-to-Y (bottom-to-top) cone to a segment with the given radius
    Transform y_to_y_cone_to_segment_transform(const LineSegment&, float radius);

    // ----- VecX/MatX helpers -----

    // returns a transform matrix that rotates `dir1` to point in the same direction as `dir2`
    Mat4 mat4_transform_between_directions(const Vec3& dir1, const Vec3& dir2);

    // returns euler angles for performing an intrinsic, step-by-step, rotation about X, Y, and then Z
    EulerAngles extract_eulers_xyz(const Quat&);

    inline Vec3 transform_point(const Mat4& mat, const Vec3& point)
    {
        return Vec3{mat * Vec4{point, 1.0f}};
    }

    inline Vec3 transform_direction(const Mat4& mat, const Vec3& direction)
    {
        return Vec3{mat * Vec4{direction, 0.0f}};
    }

    // returns a `Quat` equivalent to the given euler angles
    Quat to_world_space_rotation_quat(const EulerAngles&);

    // applies a world space rotation to the transform
    void apply_world_space_rotation(
        Transform& application_target,
        const EulerAngles& euler_angles,
        const Vec3& rotation_center
    );

    // returns the volume of a given tetrahedron, defined as 4 points in space
    float volume_of(const Tetrahedron&);

    // returns arrays that transforms cube faces from world space to projection
    // space such that the observer is looking at each face of the cube from
    // the center of the cube
    std::array<Mat4, 6> calc_cubemap_view_proj_matrices(
        const Mat4& projection_matrix,
        Vec3 cube_center
    );
}
