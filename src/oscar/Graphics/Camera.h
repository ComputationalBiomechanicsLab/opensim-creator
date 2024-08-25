#pragma once

#include <oscar/Graphics/CameraClearFlags.h>
#include <oscar/Graphics/CameraClippingPlanes.h>
#include <oscar/Graphics/CameraProjection.h>
#include <oscar/Graphics/Color.h>
#include <oscar/Maths/Angle.h>
#include <oscar/Maths/Mat4.h>
#include <oscar/Maths/Quat.h>
#include <oscar/Maths/Rect.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/Utils/CopyOnUpdPtr.h>

#include <iosfwd>
#include <optional>

namespace osc { class RenderTexture; }
namespace osc { class RenderTarget; }

namespace osc
{
    // camera
    //
    // encapsulates a camera viewport that can be drawn to, with the intention of producing
    // a 2D rendered image of the drawn elements
    class Camera {
    public:
        Camera();

        // resets the camera to default parameters
        void reset();

        // get/set the background color that the camera will clear the output with before
        // performing a draw call (assuming `CameraClearFlags::SolidColor` is set)
        Color background_color() const;
        void set_background_color(const Color&);

        // get/set the kind of projection that the camera should use when projecting view-space
        // vertices into clip space (ignored if `set_projection_matrix_override` is used)
        CameraProjection projection() const;
        void set_projection(CameraProjection);

        // get/set the height of the orthographic projection plane that the camera will use
        //
        // ignored if `projection() != CameraProjection::Orthographic`, the width of the
        // orthographic plane is calculated from the aspect ratio of the render target at
        // runtime.
        float orthographic_size() const;
        void set_orthographic_size(float);

        // get/set the vertical field-of-view angle of the viewer's projection camera
        //
        // ignored if `projection() != CameraProjection::Perspective`. The horizontal FoV is
        // calculated from the aspect ratio of the render target at runtime.
        Radians vertical_fov() const;
        void set_vertical_fov(Radians);

        // get/set the distance, in worldspace units, between both the camera and the nearest
        // clipping plane, and the camera and the farthest clipping plane
        CameraClippingPlanes clipping_planes() const;
        void set_clipping_planes(CameraClippingPlanes);

        // get/set the distance, in worldspace units, between the camera and the nearest
        // clipping plane
        float near_clipping_plane() const;
        void set_near_clipping_plane(float);

        // get/set the distance, in worldspace units, between the camera and the farthest
        // clipping plane
        float far_clipping_plane() const;
        void set_far_clipping_plane(float);

        // get/set the camera's clear flags, which affect how/if the camera clears the output
        // during a call to `graphics::draw`
        CameraClearFlags clear_flags() const;
        void set_clear_flags(CameraClearFlags);

        // get/set where on the output that this `Camera` should rasterize its pixels
        // during a call to `graphics::draw`
        //
        // the rectangle is defined in screen space, which:
        //
        // - is measured in pixels
        // - starts in the bottom-left corner
        // - ends in the top-right corner
        //
        // `std::nullopt` implies that the camera should render to the full extents
        // of the screen or render target
        std::optional<Rect> pixel_rect() const;
        void set_pixel_rect(std::optional<Rect>);

        // get/set the scissor rectangle, which tells the renderer to only clear and/or
        // render fragments (pixels) that occur within the given rectangle
        //
        // the rectangle is defined in screen space, which:
        //
        // - is measured in pixels
        // - starts in the bottom-left corner
        // - ends in the top-right corner
        //
        // `std::nullopt` implies that the camera should clear (if applicable) the entire
        // output, followed by writing output fragments to the output pixel rectangle
        // with no scissoring
        //
        // the usefulness of scissor testing is that it can be used to:
        //
        // - limit running an expensive fragment shader to a smaller subspace
        // - only draw sub-parts of a scene without having to recompute transforms or render it differently etc.
        // - only clear + draw to a smaller subspace of the output (e.g. writing to sub-sections of a larger UI)
        std::optional<Rect> scissor_rect() const;
        void set_scissor_rect(std::optional<Rect>);

        // get/set the worldspace position of this `Camera`
        Vec3 position() const;
        void set_position(const Vec3&);

        // get/set the orientation of this `Camera`
        //
        // the default/identity orientation of the camera has it pointing along `-Z`, with
        // `+Y` pointing "up"
        Quat rotation() const;
        void set_rotation(const Quat&);

        // get/set the direction in which this `Camera` is pointing
        //
        // care: This is a convenience method. `Camera` actually stores a rotation, not this
        //       direction vector. The implementation assumes that the direction is along `-Z`
        //       and that `+Y` is "up", followed by figuring out what rotation is necessary to
        //       point it along directions get/set via these methods.
        //
        //       Therefore, if you want to "roll" the camera (i.e. where `+Y` isn't "up"), you
        //       should directly manipulate the rotation of this camera, rather than trying to
        //       play with this method.
        Vec3 direction() const;
        void set_direction(const Vec3&);

        // returns the "up" direction of this camera
        Vec3 upwards_direction() const;

        // returns the matrix that this camera uses to transform world-space locations into
        // view-space
        //
        // world-space and view-space operate with the same units-of-measure, handedness, etc.
        // but view-space places the camera at `(0, 0, 0)`
        Mat4 view_matrix() const;

        // get/set matrices that override the default view matrix that this `Camera` uses
        //
        // by default, `Camera` computes its view matrix from its position and rotation, but
        // it's sometimes necessary/handy to override this default behavior.
        std::optional<Mat4> view_matrix_override() const;
        void set_view_matrix_override(std::optional<Mat4>);

        // returns the matrix that this camera uses to transform view-space locations into
        // clip-space.
        //
        // clip-space is defined such that there exists a unit cube in it that eventually
        // projects onto screen space in the following way:
        //
        // - `( 0,  0,  0)` is the center of the screen
        // - `(-1, -1, -1)` is the bottom-left, and closest part, of the screen
        // - `(+1, +1, +1)` is the top-right, and farthest part, of the screen
        //
        // anything that projects into clip space but doesn't land within that cube won't be
        // drawn to the output. The XY component of fragments that land within clip space
        // are transformed into screen space and drawn to the output pixel rectangle (assuming
        // they also pass the scissor test). The Z component of things that land within clip
        // space are written to the depth buffer if the `Material` that's being drawn enables
        // this behavior (and there's a depth buffer attached to the render target)
        Mat4 projection_matrix(float aspect_ratio) const;
        std::optional<Mat4> projection_matrix_override() const;
        void set_projection_matrix_override(std::optional<Mat4>);

        // returns the equivalent of `projection_matrix(aspect_ratio) * view_matrix()`
        Mat4 view_projection_matrix(float aspect_ratio) const;

        // returns the equivalent of `inverse(view_projection_matrix(aspect_ratio))`
        Mat4 inverse_view_projection_matrix(float aspect_ratio) const;

        // flushes any rendering commands that were queued against this camera
        //
        // after this call completes, the output texture, or screen, should contain
        // the rendered geometry
        void render_to_screen();
        void render_to(RenderTexture&);
        void render_to(const RenderTarget&);

    private:
        friend bool operator==(const Camera&, const Camera&);
        friend std::ostream& operator<<(std::ostream&, const Camera&);
        friend class GraphicsBackend;

        class Impl;
        CopyOnUpdPtr<Impl> impl_;
    };

    bool operator==(const Camera&, const Camera&);
    std::ostream& operator<<(std::ostream&, const Camera&);
}
