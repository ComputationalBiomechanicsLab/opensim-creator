#pragma once

#include <liboscar/graphics/camera_api.h>
#include <liboscar/graphics/camera_clipping_planes.h>
#include <liboscar/maths/angle.h>
#include <liboscar/maths/coordinate_direction.h>
#include <liboscar/maths/matrix4x4.h>
#include <liboscar/maths/ray.h>
#include <liboscar/maths/vector.h>

namespace osc { struct AABB; }
namespace osc { class Rect; }

namespace osc
{
    // a camera that focuses on and swivels around a focal point (e.g. for 3D model viewers)
    struct PolarPerspectiveCamera final : public CameraAPI {

        static PolarPerspectiveCamera with_radius(float radius);
        static PolarPerspectiveCamera focused_on(const AABB& aabb);

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

        void zoom_in()  { radius *= 0.8f; }
        void zoom_out() { radius *= 1.2f; }
        void focus_on(const AABB& aabb, float aspect_ratio = 1.0f);

        // autoscale znear and zfar based on the camera's distance from what it's looking at
        //
        // important for looking at tiny/large scenes. znear and zfar dictates
        // both the culling planes of the camera *and* rescales the Z values of elements
        // in the scene. If the znear-to-zfar range is too large then Z-fighting will happen
        // and the scene will look wrong.
        void rescale_znear_and_zfar_based_on_radius();

        Matrix4x4 view_matrix() const override;
        Matrix4x4 projection_matrix(float aspect_ratio) const override;

        // uses this camera's transform to project a world space point
        // onto the given viewport rectangle.
        Vector2 project_onto_viewport(const Vector3& world_space_position, const Rect& viewport_rect) const override;

        Vector3 position() const override;
        Vector3 forward() const override;

        CameraClippingPlanes clipping_planes() const override;

        // converts a `pos` (top-left) in the output `dimensions` into a `Ray` in world space by unprojection
        Ray unproject_topleft_position_to_world_ray(Vector2 pos, Vector2 dimensions) const override;

        // Returns the height of the view frustum in world units at a given depth from
        // the camera origin (also in world units).
        float frustum_height_at_depth(float depth) const;

        void focus_along(CoordinateDirection);

        friend bool operator==(const PolarPerspectiveCamera&, const PolarPerspectiveCamera&) = default;

        float radius = 1.0f;
        Radians theta = Degrees{45.0f};
        Radians phi = Degrees{45.0f};
        Vector3 focus_point{};
        Radians vertical_field_of_view = Degrees{35.0f};
        float znear = 0.1f;
        float zfar = 100.0f;
    };
}
