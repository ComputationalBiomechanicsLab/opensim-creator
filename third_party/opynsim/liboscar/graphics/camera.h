#pragma once

#include <liboscar/graphics/camera_v2.h>
#include <liboscar/graphics/camera_clipping_planes.h>
#include <liboscar/graphics/camera_projection.h>
#include <liboscar/graphics/clear_flags.h>
#include <liboscar/graphics/color.h>
#include <liboscar/maths/angle.h>
#include <liboscar/maths/matrix4x4.h>
#include <liboscar/maths/quaternion.h>
#include <liboscar/maths/rect.h>
#include <liboscar/maths/vector.h>
#include <liboscar/utilities/copy_on_upd_shared_value.h>

#include <iosfwd>
#include <optional>

namespace osc { class RenderQueue; }
namespace osc { class RenderTexture; }
namespace osc { class RenderTarget; }
namespace osc { class SharedDepthStencilRenderBuffer; }

namespace osc
{
    // camera
    //
    // represents a camera in world space that can rasterize drawcalls issued
    // via `graphics::draw` to a 2D render target.
    class Camera : public CameraV2 {
    public:
        Camera();

        // resets the camera to default parameters
        void reset();

        // get/set the background color that the camera will clear the output with before
        // performing a draw call (assuming `CameraClearFlags::SolidColor` is set)
        Color background_color() const;
        void set_background_color(const Color&);

        // get/set the camera's clear flags, which affect how/if the renderer clears the output
        // during a call to `render`
        ClearFlags clear_flags() const;
        void set_clear_flags(ClearFlags);

        // get/set where on the output that this `Camera` should rasterize its pixels
        // during a call to `graphics::draw`
        //
        // the rectangle is defined in screen space, which:
        //
        // - is measured in device-independent pixels
        // - starts in the bottom-left corner
        // - ends in the top-right corner
        //
        // `std::nullopt` implies that the camera should render to the full extents
        // of the render target
        std::optional<Rect> pixel_rect() const;
        void set_pixel_rect(std::optional<Rect>);

        // get/set the scissor rectangle, which tells the renderer to only clear and/or
        // render fragments (pixels) that occur within the given rectangle
        //
        // the rectangle is defined in screen space, which:
        //
        // - is measured in device-independent pixels
        // - starts in the bottom-left corner
        // - ends in the top-right corner
        //
        // `std::nullopt` implies that the camera should clear (if applicable) the entire
        // output, followed by writing output fragments to the output pixel rectangle
        // with no scissoring
        //
        // the usefulness of scissor testing is that it can be used to:
        //
        // - limit running an expensive fragment shader to a smaller region
        // - only draw sub-parts of a scene without having to recompute transforms or render it differently etc.
        // - only clear + draw to a smaller region of the output (e.g. writing to subsections of a larger UI)
        std::optional<Rect> scissor_rect() const;
        void set_scissor_rect(std::optional<Rect>);

        // Returns a reference to the camera's `RenderQueue`.
        RenderQueue& upd_render_queue();

        // flushes and renders any queued drawcalls from `graphics::draw(...)` to the
        // main application window.
        void render_to_main_window();

        // flushes and renders any queued drawcalls from `graphics::draw(...)` to `render_texture`.
        void render_to(RenderTexture& render_texture);

        // flushes and renders any queued drawcalls from `graphics::draw(...)` to `render_target`.
        void render_to(const RenderTarget& render_target);

        // flushes and renders any queued drawcalls from `graphics::draw(...)` to `shared_depth_stencil_buffer`.
        //
        // the resulting render pass is a depth-only render
        void render_to(SharedDepthStencilRenderBuffer& shared_depth_stencil_buffer);
    private:
        friend bool operator==(const Camera&, const Camera&);
        friend std::ostream& operator<<(std::ostream&, const Camera&);

        class Impl;
        CopyOnUpdSharedValue<Impl> impl_;
    };

    bool operator==(const Camera&, const Camera&);
    std::ostream& operator<<(std::ostream&, const Camera&);
}
