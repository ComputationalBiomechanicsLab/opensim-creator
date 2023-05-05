#include "RenderTarget.hpp"

#include "src/Graphics/RenderTargetColorAttachment.hpp"
#include "src/Graphics/RenderTargetDepthAttachment.hpp"

#include <utility>
#include <vector>

osc::RenderTarget::RenderTarget(
	std::vector<RenderTargetColorAttachment> colors_,
	RenderTargetDepthAttachment depth_) :

	colors{std::move(colors_)},
	depth{std::move(depth_)}
{
}