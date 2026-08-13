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

        friend bool operator==(const PolarPerspectiveCamera&, const PolarPerspectiveCamera&) = default;

        // reset the camera to its initial state
        void reset();

        Matrix4x4 view_matrix() const override;
        Matrix4x4 projection_matrix(float aspect_ratio) const override;
        Vector3 position() const override;
        Vector3 forward() const override;
        CameraClippingPlanes clipping_planes() const override;

        // note: relative deltas here are relative to whatever viewport the camera
        // is handling.
        //
        // e.g. moving a mouse 400px in X in a viewport that is 800px wide should
        //      have a delta.x of 0.5f

        void pan(float aspect_ratio, Vector2 mouse_delta);  // pan origin along current view plane
        void drag(Vector2 mouse_delta);                     // spin around origin (constant radius)
        void zoom_in()  { radius *= 0.8f; }
        void zoom_out() { radius *= 1.2f; }
        void focus_on(const AABB& aabb, float aspect_ratio = 1.0f);
        void focus_along(CoordinateDirection);

        /// Returns a vector in ui space where this camera would project
        /// `world_position` (in world space) into `ui_rect` (in ui space).
        Vector2 world_to_ui(const Vector3& world_position, const Rect& ui_rect) const override;

        /// Returns a `Ray` in world space that represents where `ui_position` (in
        /// ui space, Z unknown) would shoot along the view space's Z axis, assuming
        /// `ui_rect` represents the ui space viewport that this camera projects to.
        Ray ui_to_world(const Vector2& ui_position, const Rect& ui_rect) const override;

        // Returns the height of the view frustum in world units at a given depth from
        // the camera origin (also in world units).
        float frustum_height_at_depth(float depth) const;

        float radius = 1.0f;
        Radians theta = Degrees{45.0f};
        Radians phi = Degrees{45.0f};
        Vector3 focus_point{};
        Radians vertical_field_of_view = Degrees{35.0f};
        float znear = 0.1f;
        float zfar = 100.0f;
    };
}
