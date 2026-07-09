#pragma once

#include <liboscar/graphics/camera_clipping_planes.h>
#include <liboscar/graphics/camera_projection.h>
#include <liboscar/maths/angle.h>
#include <liboscar/maths/matrix4x4.h>
#include <liboscar/maths/vector.h>
#include <liboscar/utilities/copy_on_upd_shared_value.h>

#include <optional>

namespace osc
{
    /// Represents a camera at a `position` in world space, pointing toward `direction`
    /// and oriented such that it is `up`.
    class CameraV2 final {
    public:
        CameraV2();

        // resets the camera to default parameters
        void reset();

        // get/set the kind of projection that the camera should use when projecting view space
        // vertices into clip space (ignored if `set_projection_matrix_override` is used)
        CameraProjection projection() const;
        void set_projection(CameraProjection);

        // get/set the height of the orthographic projection plane that the camera will use
        //
        // undefined behavior if `projection() != CameraProjection::Orthographic`, or the
        // projection matrix has been overridden with `set_projection_matrix_override`, the
        // width of the orthographic plane is calculated from the aspect ratio of the render
        // target at runtime.
        float orthographic_size() const;
        void set_orthographic_size(float);

        // get/set the vertical field-of-view angle of the viewer's projection camera
        //
        // undefined behavior if `projection() != CameraProjection::Perspective` or the projection matrix
        // has been overridden with `set_projection_matrix_override`.
        Radians vertical_field_of_view() const;
        void set_vertical_field_of_view(Radians);

        // returns the horizontal field-of-view angle of the viewer's projection camera, assuming
        // it's rendering to a render target with the given `aspect_ratio`.
        //
        // undefined behavior if `projection() != CameraProjection::Perspective` or the projection matrix
        // has been overridden with `set_projection_matrix_override`.
        Radians horizontal_field_of_view(float aspect_ratio) const;

        // get/set the distance, in world space units, between both the camera and the nearest
        // clipping plane, and the camera and the farthest clipping plane
        CameraClippingPlanes clipping_planes() const;
        void set_clipping_planes(CameraClippingPlanes);

        // get/set the distance, in world space units, between the camera and the nearest
        // clipping plane
        float near_clipping_plane() const;
        void set_near_clipping_plane(float);

        // get/set the distance, in world space units, between the camera and the farthest
        // clipping plane
        float far_clipping_plane() const;
        void set_far_clipping_plane(float);

        // get/set the world space position of this `Camera`
        Vector3 position() const;
        void set_position(const Vector3&);

        // get/set the direction in which this `Camera` is pointing
        Vector3 direction() const;
        void set_direction(const Vector3&);

        // returns the upwards direction of this camera
        Vector3 up() const;
        void set_up(const Vector3&);

        // returns the matrix that this camera uses to transform world space points into
        // view space
        //
        // world space and view space operate with the same units-of-measure, handedness, etc.
        // but view space places the camera at `(0, 0, 0)`
        Matrix4x4 view_matrix() const;

        // returns the equivalent of `inverse(view_matrix())`, i.e. a matrix that transforms
        // view space points into world space points.
        Matrix4x4 inverse_view_matrix() const;

        // get/set matrices that override the default view matrix that this `Camera` uses
        //
        // by default, `Camera` computes its view matrix from its position and rotation, but
        // it's sometimes necessary/handy to override this default behavior.
        std::optional<Matrix4x4> view_matrix_override() const;
        void set_view_matrix_override(std::optional<Matrix4x4>);

        // returns the matrix that this camera uses to transform view space points into
        // clip space.
        //
        // clip space is defined such that there exists a unit cube in it that eventually
        // projects onto screen space in the following way:
        //
        // - transformed points (affine, homogeneous) are divided by their `w` component (perspective
        //   divide) to yield their normalized device coordinates (NDC).
        // - Anything outside of [{-1,-1,-1},{+1,+1,+1}] in NDC is discarded (clipping)
        // - NDC `( 0,  0,  0)` maps to the midpoint of screen space (i.e. 0.5 * {w, h})
        // - NDC `(-1, -1, -1)` maps to the bottom-left of screen space (z = -1 means 'closest')
        // - NDC `(+1, +1, +1)` maps to the top-right of screen space (z = +1 means 'farthest')
        // - Therefore, NDC is left-handed
        //
        // The XY component of fragments that land within clip space are transformed into screen
        // space and drawn to the output pixel rectangle (assuming they also pass the scissor test).
        // The Z component of things that land within the NDC cube are written to the depth buffer
        // if the `Material` that's being drawn enables this behavior (and there's a depth buffer
        // attached to the render target).
        Matrix4x4 projection_matrix(float aspect_ratio) const;
        std::optional<Matrix4x4> projection_matrix_override() const;
        void set_projection_matrix_override(std::optional<Matrix4x4>);

        // returns the equivalent of `projection_matrix(aspect_ratio) * view_matrix()`
        Matrix4x4 view_projection_matrix(float aspect_ratio) const;

        // returns the equivalent of `inverse(view_projection_matrix(aspect_ratio))`
        Matrix4x4 inverse_view_projection_matrix(float aspect_ratio) const;

    private:
        friend bool operator==(const CameraV2&, const CameraV2&);
        class Impl;
        CopyOnUpdSharedValue<Impl> impl_;
    };

    bool operator==(const CameraV2&, const CameraV2&);
}
