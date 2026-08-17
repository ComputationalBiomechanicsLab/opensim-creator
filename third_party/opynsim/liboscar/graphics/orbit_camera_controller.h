#pragma once

#include <liboscar/maths/angle.h>
#include <liboscar/maths/coordinate_direction.h>
#include <liboscar/maths/vector.h>

namespace osc { struct AABB; }
namespace osc { class Camera; }

namespace osc
{
    /// Controls a camera that orbits around a focal point, similar to an
    /// "arcball" or "turntable" camera controller in other libraries.
    struct OrbitCameraController final {

        static OrbitCameraController focused_on(
            const AABB& aabb,
            const Camera& camera,
            float aspect_ratio = 1.0f
        );

        friend bool operator==(const OrbitCameraController&, const OrbitCameraController&) = default;

        void update_camera(Camera&) const;

        void reset();
        void pan(float aspect_ratio, Vector2 normalized_delta, const Camera&);
        void drag(Vector2 normalized_delta);
        void zoom_in()  { radius *= 0.8f; }
        void zoom_out() { radius *= 1.2f; }
        void focus_on(const AABB& aabb, const Camera& camera, float aspect_ratio = 1.0f);
        void focus_along(CoordinateDirection);
        void focus_along(const Vector3& direction);

        Vector3 focus_point{};
        float radius = 1.0f;
        Radians theta = Degrees{45.0f};
        Radians phi = Degrees{45.0f};
    };
}
