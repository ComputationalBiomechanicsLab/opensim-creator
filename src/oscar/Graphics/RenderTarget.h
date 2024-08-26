#pragma once

#include <oscar/Graphics/RenderTargetColorAttachment.h>
#include <oscar/Graphics/RenderTargetDepthAttachment.h>
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
            depth_{RenderTargetDepthAttachment{}}
        {
            validate_or_throw();
        }
        explicit RenderTarget(RenderTargetColorAttachment color_attachment) :
            colors_{std::move(color_attachment)}
        {
            validate_or_throw();
        }
        explicit RenderTarget(RenderTargetDepthAttachment depth_attachment) :
            depth_{std::move(depth_attachment)}
        {
            validate_or_throw();
        }
        explicit RenderTarget(RenderTargetColorAttachment color_attachment, RenderTargetDepthAttachment depth_attachment) :
            colors_{std::move(color_attachment)},
            depth_{std::move(depth_attachment)}
        {
            validate_or_throw();
        }
        explicit RenderTarget(RenderTargetColorAttachment color0_attachment, RenderTargetColorAttachment color1_attachment, RenderTargetDepthAttachment depth_attachment) :
            colors_{std::move(color0_attachment), std::move(color1_attachment)},
            depth_{std::move(depth_attachment)}
        {
            validate_or_throw();
        }
        explicit RenderTarget(RenderTargetColorAttachment color0_attachment, RenderTargetColorAttachment color1_attachment, RenderTargetColorAttachment color2_attachment, RenderTargetDepthAttachment depth_attachment) :
            colors_{std::move(color0_attachment), std::move(color1_attachment), std::move(color2_attachment)},
            depth_{std::move(depth_attachment)}
        {
            validate_or_throw();
        }

        std::span<const RenderTargetColorAttachment> color_attachments() const { return colors_; }
        const std::optional<RenderTargetDepthAttachment>& depth_attachment() const { return depth_; }
        Vec2i dimensions() const
        {
            return depth_ ? depth_->buffer.dimensions() : colors_.front().buffer.dimensions();
        }
        void validate_or_throw() const;

    private:
        std::vector<RenderTargetColorAttachment> colors_;
        std::optional<RenderTargetDepthAttachment> depth_;
    };
}
