#pragma once

#include <liboscar/Maths/AABB.h>
#include <liboscar/Maths/Angle.h>
#include <liboscar/Maths/EulerAngles.h>
#include <liboscar/Maths/Matrix3x3.h>
#include <liboscar/Maths/Matrix4x4.h>
#include <liboscar/Maths/Quaternion.h>
#include <liboscar/Maths/Ray.h>
#include <liboscar/Maths/Rect.h>
#include <liboscar/Maths/Sphere.h>
#include <liboscar/Maths/Transform.h>
#include <liboscar/Maths/TransformFunctions.h>
#include <liboscar/Maths/Vec.h>
#include <liboscar/Maths/Vector2.h>
#include <liboscar/Maths/Vector3.h>
#include <liboscar/Maths/Vector4.h>

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

    // returns a normalized device coordinate-like (NDC-like) point converted from a point
    // defined in a normalized y-points-down space.
    //
    // - input point should have origin in top-left, Y goes down
    // - input point should have normalized range: (0, 0) is top-left, (+1, +1) is bottom-right
    // - output point's origin will be in the middle of the input space, Y goes up
    // - output point's range will be origin-centered: (-1, -1) is bottom-left, (+1, +1) is top-right
    Vector2 topleft_normalized_point_to_ndc(Vector2 normalized_point);

    // returns a normalized y-points-down point converted from a point defined in a
    // normalized device coordinate-like (NDC-like) space.
    //
    // - input point should have origin in the middle, Y goes up
    // - input point should have an origin-centered range: (-1, -1) is bottom-left, (+1, +1) is top-right
    // - output point origin will have an origin in the top-left, Y goes down
    // - output point has range: (0, 0) for top-left, (1, 1) for bottom-right
    Vector2 ndc_point_to_topleft_normalized(Vector2 ndc_point);

    // returns an NDC affine point (i.e. {x, y, z, 1.0}) converted from a point
    // defined in a normalized y-points-down space.
    //
    // - input point should have origin in top-left, Y goes down
    // - input point should have normalized range: (0, 0) is top-left, (+1, +1) is bottom-right
    // - output point origin will have an origin in the top-left, Y goes down
    // - output point has range: (0, 0) for top-left, (1, 1) for bottom-right
    // - output point will have a `z` of `-1.0f` (i.e. nearest depth)
    Vector4 topleft_normalized_point_to_ndc_cube(Vector2 normalized_point);

    // "un-project" a point defined in a normalized y-points-down space into world
    // space, assuming a perspective projection
    //
    // - input point should have origin in top-left, Y goes down
    // - input point should have normalized range: (0, 0) is top-left, (+1, +1) is bottom-right
    // - `camera_world_space_origin` is the position of the camera in world space
    // - `camera_view_matrix` transforms points from world space to view space
    // - `camera_proj_matrix` transforms points from view space to world space
    Ray perspective_unproject_topleft_normalized_pos_to_world(
        Vector2 normalized_point,
        Vector3 camera_world_space_origin,
        const Matrix4x4& camera_view_matrix,
        const Matrix4x4& camera_proj_matrix
    );

    // returns a rectangular region defined in, and bounded by, normalized device coordinates
    // (NDCs) mapped from a region defined in a normalized y-points-down space bounded
    // by `viewport`.
    Rect ndc_rect_to_topleft_viewport_rect(const Rect& ndc_rect, const Rect& viewport);

    // returns the position where `world_space_position` would occur when projected via the
    // given `view_matrix` and `projection_matrix`es onto `viewport_rect`.
    Vector2 project_onto_viewport_rect(
        const Vector3& world_space_position,
        const Matrix4x4& view_matrix,
        const Matrix4x4& projection_matrix,
        const Rect& viewport_rect
    );


    // ----- `Sphere` helpers -----

    // returns a `Sphere` that loosely bounds the given `Vector3`s
    Sphere bounding_sphere_of(std::span<const Vector3>);

    // returns a `Sphere` that loosely bounds the given `AABB`
    Sphere bounding_sphere_of(const AABB&);

    // returns an `AABB` that tightly bounds the `Sphere`
    AABB bounding_aabb_of(const Sphere&);


    // ----- `Ray` helpers -----

    // returns a `Ray` that has been transformed by the `Mat4`
    Ray transform_ray(const Ray&, const Matrix4x4&);

    // returns a `Ray` that has been transformed by the inverse of the supplied `Transform`
    Ray inverse_transform_ray(const Ray&, const Transform&);


    // ----- `Disc` helpers -----

    // returns a `Mat4` that maps one `Disc` to another `Disc`
    Matrix4x4 matrix4x4_transform_between(const Disc&, const Disc&);


    // ----- `Segment` helpers -----

    // returns a transform matrix that maps a path segment to another path segment
    Matrix4x4 matrix4x4_transform_between(const LineSegment&, const LineSegment&);

    // returns a `Transform` that maps a path segment to another path segment
    Transform transform_between(const LineSegment&, const LineSegment&);

    // returns a `Transform` that maps a Y-to-Y (bottom-to-top) cylinder to a segment with the given radius
    Transform cylinder_to_line_segment_transform(const LineSegment&, float radius);

    // returns a `Transform` that maps a Y-to-Y (bottom-to-top) cone to a segment with the given radius
    Transform y_to_y_cone_to_segment_transform(const LineSegment&, float radius);

    // ----- VecX/MatX helpers -----

    // returns a transform matrix that rotates `dir1` to point in the same direction as `dir2`
    Matrix4x4 matrix4x4_transform_between_directions(const Vector3& dir1, const Vector3& dir2);

    // returns euler angles for performing an intrinsic, step-by-step, rotation about X, Y, and then Z
    EulerAngles extract_eulers_xyz(const Quaternion&);

    inline Vector3 transform_point(const Matrix4x4& mat, const Vector3& point)
    {
        return Vector3{mat * Vector4{point, 1.0f}};
    }

    inline Vector3 transform_direction(const Matrix4x4& mat, const Vector3& direction)
    {
        return Vector3{mat * Vector4{direction, 0.0f}};
    }

    // returns a `Quaternion` equivalent to the given euler angles
    Quaternion to_world_space_rotation_quaternion(const EulerAngles&);

    // applies a world space rotation to the transform
    void apply_world_space_rotation(
        Transform& application_target,
        const EulerAngles& euler_angles,
        const Vector3& rotation_center
    );

    // returns the volume of a given tetrahedron, defined as 4 points in space
    float volume_of(const Tetrahedron&);

    // returns arrays that transforms cube faces from world space to projection
    // space such that the observer is looking at each face of the cube from
    // the center of the cube
    std::array<Matrix4x4, 6> calc_cubemap_view_proj_matrices(
        const Matrix4x4& projection_matrix,
        Vector3 cube_center
    );
}
