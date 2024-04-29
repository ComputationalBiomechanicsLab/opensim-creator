#pragma once

#include <oscar/Graphics/RenderTargetColorAttachment.h>
#include <oscar/Graphics/RenderTargetDepthAttachment.h>

#include <utility>
#include <vector>

namespace osc
{
    struct RenderTarget final {
        RenderTarget(
            std::vector<RenderTargetColorAttachment> colors_,
            RenderTargetDepthAttachment attachment_) :

            colors{std::move(colors_)},
            depth{std::move(attachment_)}
        {}

        std::vector<RenderTargetColorAttachment> colors;
        RenderTargetDepthAttachment depth;
    };
}
