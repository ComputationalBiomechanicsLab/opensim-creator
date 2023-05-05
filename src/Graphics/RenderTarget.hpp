#pragma once

#include "src/Graphics/RenderTargetColorAttachment.hpp"
#include "src/Graphics/RenderTargetDepthAttachment.hpp"

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