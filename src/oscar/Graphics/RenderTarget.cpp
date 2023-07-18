#include "RenderTarget.hpp"

#include "oscar/Graphics/RenderTargetColorAttachment.hpp"
#include "oscar/Graphics/RenderTargetDepthAttachment.hpp"

#include <utility>
#include <vector>

osc::RenderTarget::RenderTarget(
    std::vector<RenderTargetColorAttachment> colors_,
    RenderTargetDepthAttachment depth_) :

    colors{std::move(colors_)},
    depth{std::move(depth_)}
{
}
