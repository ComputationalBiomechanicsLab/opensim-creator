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

    // returns an XY NDC point converted from a point defined in screenspace
    //
    // - input screen point has origin in top-left, Y goes down
    // - input screen point has range: (0,0) is top-left, (1, 1) is bottom-right
    // - output NDC point has origin in middle, Y goes up
    // - output NDC point has range: (-1, 1) is top-left, (1, -1) is bottom-right
    Vec2 topleft_relative_pos_to_ndc_point(Vec2 relative_pos);

    // returns an XY top-left relative point converted from the given NDC point
    //
    // - input NDC point has origin in the middle, Y goes up
    // - input NDC point has range: (-1, -1) for top-left, (1, -1) is bottom-right
    // - output point has origin in the top-left, Y goes down
    // - output point has range: (0, 0) for top-left, (1, 1) for bottom-right
    Vec2 ndc_point_to_topleft_relative_pos(Vec2 ndc_pos);

    // returns an NDC affine point vector (i.e. {x, y, z, 1.0}) converted from
    // a point defined in screenspace
    //
    // - input screen point has origin in top-left, Y goes down
    // - input screen point has range: (0,0) is top-left, (1, 1) is bottom-right
    // - output NDC point has origin in middle, Y goes up
    // - output NDC point has range -1 to +1 in each dimension
    // - output assumes Z is "at the front of the cube" (Z = -1.0f)
    // - output will therefore be: {xNDC, yNDC, -1.0f, 1.0f}
    Vec4 topleft_relative_pos_to_ndc_cube(Vec2 relative_pos);

    // "un-project" a point defined in screenspace into 3D world-space, assuming a
    // perspective camera
    //
    // - input screen point has origin in top-left, Y goes down
    // - input screen point has range: (0,0) is top-left, (1, 1) is bottom-right
    // - `camera_worldspace_origin` is the location of the camera in world space
    // - `camera_view_matrix` transforms points from world-space to view-space
    // - `camera_proj_matrix` transforms points from view-space to world-space
    Line perspective_unproject_topleft_screen_pos_to_world_ray(
        Vec2 relative_pos,
        Vec3 camera_worldspace_origin,
        const Mat4& camera_view_matrix,
        const Mat4& camera_proj_matrix
    );

    // returns a rect, created by mapping a Normalized Device Coordinates (NDC) rect
    // (i.e. -1.0 to 1.0) to a rect defined in screenspace (pixel units, topleft == (0, 0))
    Rect ndc_rect_to_screenspace_viewport_rect(const Rect& ndc_rect, const Rect& viewport);

    // returns the location where `worldspace_location` would occur when projected via the
    // given `view_matrix` and `projection_matrix`es onto `screen_rect`
    Vec2 project_onto_screen_rect(
        const Vec3& worldspace_location,
        const Mat4& view_matrix,
        const Mat4& projection_matrix,
        const Rect& screen_rect
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
    Quat to_worldspace_rotation_quat(const EulerAngles&);

    // applies a world-space rotation to the transform
    void apply_worldspace_rotation(
        Transform& application_target,
        const EulerAngles& euler_angles,
        const Vec3& rotation_center
    );

    // returns the volume of a given tetrahedron, defined as 4 points in space
    float volume_of(const Tetrahedron&);

    // returns arrays that transforms cube faces from worldspace to projection
    // space such that the observer is looking at each face of the cube from
    // the center of the cube
    std::array<Mat4, 6> calc_cubemap_view_proj_matrices(
        const Mat4& projection_matrix,
        Vec3 cube_center
    );
}
