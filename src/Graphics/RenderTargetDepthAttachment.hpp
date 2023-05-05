#pragma once

#include "src/Graphics/RenderTargetAttachment.hpp"

namespace osc
{
	struct RenderTargetDepthAttachment final : public RenderTargetAttachment {
		using RenderTargetAttachment::RenderTargetAttachment;
	};
}