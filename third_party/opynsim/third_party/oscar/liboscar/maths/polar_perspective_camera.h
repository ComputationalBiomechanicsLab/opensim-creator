#pragma once

#include <liboscar/maths/angle.h>
#include <liboscar/maths/matrix4x4.h>
#include <liboscar/maths/ray.h>
#include <liboscar/maths/vector2.h>
#include <liboscar/maths/vector3.h>

namespace osc { struct AABB; }
namespace osc { class Rect; }

namespace osc
{
    // a camera that focuses on and swivels around a focal point (e.g. for 3D model viewers)
    struct PolarPerspectiveCamera final {

        PolarPerspectiveCamera();

        // reset the camera to its initial state
        void reset();

        // note: relative deltas here are relative to whatever viewport the camera
        // is handling.
        //
        // e.g. moving a mouse 400px in X in a viewport that is 800px wide should
        //      have a delta.x of 0.5f

        // pan: pan along the current view plane
        void pan(float aspect_ratio, Vector2 mouse_delta);

        // drag: spin the view around the origin, such that the distance between
        //       the camera and the origin remains constant
        void drag(Vector2 mouse_delta);

        // autoscale znear and zfar based on the camera's distance from what it's looking at
        //
        // important for looking at tiny/large scenes. znear and zfar dictates
        // both the culling planes of the camera *and* rescales the Z values of elements
        // in the scene. If the znear-to-zfar range is too large then Z-fighting will happen
        // and the scene will look wrong.
        void rescale_znear_and_zfar_based_on_radius();

        Matrix4x4 view_matrix() const;
        Matrix4x4 projection_matrix(float aspect_ratio) const;

        // uses this camera's transform to project a world space point
        // onto the given viewport rectangle.
        Vector2 project_onto_viewport(const Vector3& world_space_position, const Rect& viewport_rect) const;

        Vector3 position() const;

        // converts a `pos` (top-left) in the output `dimensions` into a `Ray` in world space by unprojection
        Ray unproject_topleft_position_to_world_ray(Vector2 pos, Vector2 dimensions) const;

        // Returns the height of the view frustum in world units at a given depth from
        // the camera origin (also in world units).
        float frustum_height_at_depth(float depth) const;

        friend bool operator==(const PolarPerspectiveCamera&, const PolarPerspectiveCamera&) = default;

        float radius;
        Radians theta;
        Radians phi;
        Vector3 focus_point;
        Radians vertical_field_of_view;
        float znear;
        float zfar;
    };

    PolarPerspectiveCamera create_camera_with_radius(float);
    PolarPerspectiveCamera create_camera_focused_on(const AABB&);
    Vector3 recommended_light_direction(const PolarPerspectiveCamera&);
    void focus_along_axis(PolarPerspectiveCamera&, size_t, bool negate = false);
    void focus_along_x(PolarPerspectiveCamera&);
    void focus_along_minus_x(PolarPerspectiveCamera&);
    void focus_along_y(PolarPerspectiveCamera&);
    void focus_along_minus_y(PolarPerspectiveCamera&);
    void focus_along_z(PolarPerspectiveCamera&);
    void focus_along_minus_z(PolarPerspectiveCamera&);
    void zoom_in(PolarPerspectiveCamera&);
    void zoom_out(PolarPerspectiveCamera&);
    void reset(PolarPerspectiveCamera&);
    void auto_focus(PolarPerspectiveCamera&, const AABB& element_aabb, float aspect_ratio = 1.0f);
}
