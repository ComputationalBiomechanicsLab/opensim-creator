#pragma once

#include <oscar/Graphics/RenderTargetColorAttachment.h>
#include <oscar/Graphics/RenderTargetDepthAttachment.h>

#include <vector>

namespace osc
{
    struct RenderTarget final {
        RenderTarget(
            std::vector<RenderTargetColorAttachment>,
            RenderTargetDepthAttachment
        );

        std::vector<RenderTargetColorAttachment> colors;
        RenderTargetDepthAttachment depth;
    };
}
