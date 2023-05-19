#pragma once

#include "oscar/Graphics/RenderTargetAttachment.hpp"

namespace osc
{
	struct RenderTargetDepthAttachment final : public RenderTargetAttachment {
		using RenderTargetAttachment::RenderTargetAttachment;
	};

	bool operator==(RenderTargetDepthAttachment const&, RenderTargetDepthAttachment const&) noexcept;
	bool operator!=(RenderTargetDepthAttachment const&, RenderTargetDepthAttachment const&) noexcept;
}