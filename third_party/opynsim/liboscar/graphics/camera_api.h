#pragma once

#include <liboscar/graphics/camera_clipping_planes.h>
#include <liboscar/maths/matrix4x4.h>
#include <liboscar/maths/ray.h>
#include <liboscar/maths/rect.h>
#include <liboscar/maths/vector.h>

namespace osc
{
    /// An interface to an object that can operate like a `Camera`.
    ///
    /// This enables alternative implementations of `Camera` to be used
    /// by generic code.
    class CameraAPI {
    protected:
        CameraAPI() = default;
        CameraAPI(const CameraAPI&) = default;
        CameraAPI(CameraAPI&&) noexcept = default;
    public:
        virtual ~CameraAPI() noexcept = default;
    protected:
        CameraAPI& operator=(const CameraAPI&) = default;
        CameraAPI& operator=(CameraAPI&&) noexcept = default;

    public:
        friend bool operator==(const CameraAPI&, const CameraAPI&) = default;

        /// Returns the position of the camera in world space.
        virtual Vector3 position() const = 0;

        /// Returns the direction vector of the camera in world space.
        virtual Vector3 forward() const = 0;

        /// Returns the clipping planes of the camera in world space.
        virtual CameraClippingPlanes clipping_planes() const = 0;

        /// Returns a transform matrix that t
        virtual Matrix4x4 view_matrix() const = 0;
        virtual Matrix4x4 projection_matrix(float aspect_ratio) const = 0;

        virtual Vector2 project_onto_viewport(
            const Vector3& world_space_position,
            const Rect& viewport_rect
        ) const = 0;

        virtual Ray unproject_topleft_position_to_world_ray(
            Vector2 position,
            Vector2 dimensions
        ) const = 0;
    };
}