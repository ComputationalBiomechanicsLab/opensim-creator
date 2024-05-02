#pragma once

#include <oscar/Maths/Angle.h>
#include <oscar/Maths/Line.h>
#include <oscar/Maths/Mat4.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/Maths/Vec3.h>

namespace osc { struct AABB; }
namespace osc { struct Rect; }

namespace osc
{
    // a camera that focuses on and swivels around a focal point (e.g. for 3D model viewers)
    struct PolarPerspectiveCamera final {

        PolarPerspectiveCamera();

        // reset the camera to its initial state
        void reset();

        // note: relative deltas here are relative to whatever "screen" the camera
        // is handling.
        //
        // e.g. moving a mouse 400px in X in a screen that is 800px wide should
        //      have a delta.x of 0.5f

        // pan: pan along the current view plane
        void pan(float aspect_ratio, Vec2 mouse_delta);

        // drag: spin the view around the origin, such that the distance between
        //       the camera and the origin remains constant
        void drag(Vec2 mouse_delta);

        // autoscale znear and zfar based on the camera's distance from what it's looking at
        //
        // important for looking at extremely small/large scenes. znear and zfar dictates
        // both the culling planes of the camera *and* rescales the Z values of elements
        // in the scene. If the znear-to-zfar range is too large then Z-fighting will happen
        // and the scene will look wrong.
        void rescale_znear_and_zfar_based_on_radius();

        Mat4 view_matrix() const;
        Mat4 projection_matrix(float aspect_ratio) const;

        // project's a worldspace coordinate onto a screen-space rectangle
        Vec2 project_onto_screen_rect(const Vec3& worldspace_location, const Rect& screen_rect) const;

        Vec3 position() const;

        // converts a `pos` (top-left) in the output `dimensions` into a line in worldspace by unprojection
        Line unproject_topleft_pos_to_world_ray(Vec2 pos, Vec2 dimensions) const;

        friend bool operator==(const PolarPerspectiveCamera&, const PolarPerspectiveCamera&) = default;

        float radius;
        Radians theta;
        Radians phi;
        Vec3 focus_point;
        Radians vertical_fov;
        float znear;
        float zfar;
    };

    PolarPerspectiveCamera create_camera_with_radius(float);
    PolarPerspectiveCamera create_camera_focused_on(const AABB&);
    Vec3 recommended_light_direction(const PolarPerspectiveCamera&);
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
