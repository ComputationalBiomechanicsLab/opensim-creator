#pragma once

#include <oscar/Graphics/RenderTargetColorAttachment.h>
#include <oscar/Graphics/RenderTargetDepthStencilAttachment.h>
#include <oscar/Maths/Vec2.h>

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
        Vec2i dimensions() const
        {
            return depth_ ? depth_->buffer.dimensions() : colors_.front().buffer.dimensions();
        }
        void validate_or_throw() const;

    private:
        std::vector<RenderTargetColorAttachment> colors_;
        std::optional<RenderTargetDepthStencilAttachment> depth_;
    };
}
