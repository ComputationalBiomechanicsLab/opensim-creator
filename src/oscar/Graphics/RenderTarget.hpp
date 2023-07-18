#pragma once

#include "oscar/Graphics/RenderTargetColorAttachment.hpp"
#include "oscar/Graphics/RenderTargetDepthAttachment.hpp"

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