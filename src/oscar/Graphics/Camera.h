#pragma once

#include <oscar/Graphics/CameraClearFlags.h>
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
namespace osc { struct RenderTarget; }

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

        Color background_color() const;
        void set_background_color(const Color&);

        CameraProjection projection() const;
        void set_projection(CameraProjection);

        // only used if `projection() == CameraProjection::Orthographic`
        float orthographic_size() const;
        void set_orthographic_size(float);

        // only used if `projection() == CameraProjection::Perspective`
        Radians vertical_fov() const;
        void set_vertical_fov(Radians);

        float near_clipping_plane() const;
        void set_near_clipping_plane(float);

        float far_clipping_plane() const;
        void set_far_clipping_plane(float);

        CameraClearFlags clear_flags() const;
        void set_clear_flags(CameraClearFlags);

        // where on the screen/texture that the camera should render the viewport to
        //
        // the rect uses a top-left coordinate system (in screen-space - top-left, X rightwards, Y down)
        //
        // if this is not specified, the camera will render to the full extents of the given
        // render output (entire screen, or entire render texture)
        std::optional<Rect> pixel_rect() const;
        void set_pixel_rect(std::optional<Rect>);

        // scissor testing
        //
        // This tells the rendering backend to only render the fragments that occur within
        // these bounds. It's useful when (e.g.) running an expensive fragment shader (e.g.
        // image processing kernels) where you know that only a certain subspace is actually
        // interesting (e.g. rim-highlighting only selected elements)
        std::optional<Rect> scissor_rect() const;  // std::nullopt if not scissor testing
        void set_scissor_rect(std::optional<Rect>);

        Vec3 position() const;
        void set_position(const Vec3&);

        // get rotation (from the assumed "default" rotation of the camera pointing towards -Z, Y is up)
        Quat rotation() const;
        void set_rotation(const Quat&);

        // careful: the camera doesn't *store* a direction vector - it assumes the direction is along -Z,
        // and that +Y is "upwards" and figures out how to rotate from that to your desired direction
        //
        // if you want to "roll" the camera (i.e. Y isn't upwards) then use `set_rotation`
        Vec3 direction() const;
        void set_direction(const Vec3&);

        Vec3 upwards_direction() const;

        // get view matrix
        //
        // the caller can manually override the view matrix, which can be handy in certain
        // rendering scenarios
        Mat4 view_matrix() const;
        std::optional<Mat4> view_matrix_override() const;
        void set_view_matrix_override(std::optional<Mat4>);

        // projection matrix
        //
        // the caller can manually override the projection matrix, which can be handy in certain
        // rendering scenarios.
        Mat4 projection_matrix(float aspect_ratio) const;
        std::optional<Mat4> projection_matrix_override() const;
        void set_projection_matrix_override(std::optional<Mat4>);

        // returns the equivalent of projection_matrix(aspectRatio) * view_matrix()
        Mat4 view_projection_matrix(float aspect_ratio) const;

        // returns the equivalent of inverse(view_projection_matrix(aspectRatio))
        Mat4 inverse_view_projection_matrix(float aspect_ratio) const;

        // flushes any rendering commands that were queued against this camera
        //
        // after this call completes, the output texture, or screen, should contain
        // the rendered geometry
        void render_to_screen();
        void render_to(RenderTexture&);
        void render_to(RenderTarget&);

        friend void swap(Camera& a, Camera& b) noexcept
        {
            swap(a.impl_, b.impl_);
        }

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
