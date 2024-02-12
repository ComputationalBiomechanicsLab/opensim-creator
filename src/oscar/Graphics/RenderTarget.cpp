#include "RenderTarget.h"

#include <oscar/Graphics/RenderTargetColorAttachment.h>
#include <oscar/Graphics/RenderTargetDepthAttachment.h>

#include <utility>
#include <vector>

osc::RenderTarget::RenderTarget(
    std::vector<RenderTargetColorAttachment> colors_,
    RenderTargetDepthAttachment depth_) :

    colors{std::move(colors_)},
    depth{std::move(depth_)}
{
}
