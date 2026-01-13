#pragma once

#include <liboscar/graphics/RenderTargetColorAttachment.h>
#include <liboscar/graphics/RenderTargetDepthStencilAttachment.h>
#include <liboscar/maths/Vector2.h>

#include <optional>
#include <span>
#include <utility>
#include <vector>

namespace osc
{
    class RenderTarget final {
    public:
        explicit RenderTarget() :
            colors_{RenderTargetColorAttachment{}},
            depth_{RenderTargetDepthStencilAttachment{}}
        {
            validate_or_throw();
        }
        explicit RenderTarget(RenderTargetColorAttachment color_attachment) :
            colors_{std::move(color_attachment)}
        {
            validate_or_throw();
        }
        explicit RenderTarget(RenderTargetDepthStencilAttachment depth_attachment) :
            depth_{std::move(depth_attachment)}
        {
            validate_or_throw();
        }
        explicit RenderTarget(RenderTargetColorAttachment color_attachment, RenderTargetDepthStencilAttachment depth_attachment) :
            colors_{std::move(color_attachment)},
            depth_{std::move(depth_attachment)}
        {
            validate_or_throw();
        }
        explicit RenderTarget(RenderTargetColorAttachment color0_attachment, RenderTargetColorAttachment color1_attachment, RenderTargetDepthStencilAttachment depth_attachment) :
            colors_{std::move(color0_attachment), std::move(color1_attachment)},
            depth_{std::move(depth_attachment)}
        {
            validate_or_throw();
        }
        explicit RenderTarget(RenderTargetColorAttachment color0_attachment, RenderTargetColorAttachment color1_attachment, RenderTargetColorAttachment color2_attachment, RenderTargetDepthStencilAttachment depth_attachment) :
            colors_{std::move(color0_attachment), std::move(color1_attachment), std::move(color2_attachment)},
            depth_{std::move(depth_attachment)}
        {
            validate_or_throw();
        }

        std::span<const RenderTargetColorAttachment> color_attachments() const { return colors_; }
        const std::optional<RenderTargetDepthStencilAttachment>& depth_attachment() const { return depth_; }

        // Returns the dimensions of this `RenderTarget` in physical pixels.
        Vector2i pixel_dimensions() const
        {
            return depth_ ? depth_->buffer.pixel_dimensions() : colors_.front().buffer.pixel_dimensions();
        }

        // Returns the dimensions of this `RenderTarget` in device-independent pixels.
        //
        // The return value is equivalent to `pixel_dimensions() / device_pixel_ratio()`.
        Vector2 dimensions() const { return Vector2{pixel_dimensions()} / device_pixel_ratio(); }

        // Returns the ratio of the resolution of the texture in physical pixels
        // to the resolution of it in device-independent pixels.
        float device_pixel_ratio() const { return device_pixel_ratio_; }

        // Sets the device-to-pixel ratio of this `RenderTarget, which has the effect
        // of scaling the `dimensions()`.
        void set_device_pixel_ratio(float new_device_pixel_ratio) { device_pixel_ratio_ = new_device_pixel_ratio; }

        void validate_or_throw() const;

    private:
        std::vector<RenderTargetColorAttachment> colors_;
        std::optional<RenderTargetDepthStencilAttachment> depth_;
        float device_pixel_ratio_ = 1.0f;
    };
}
