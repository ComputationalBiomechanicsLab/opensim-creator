#pragma once

#include "src/Graphics/Color.hpp"
#include "src/Graphics/RenderBufferLoadAction.hpp"
#include "src/Graphics/RenderBufferStoreAction.hpp"
#include "src/Graphics/RenderTargetAttachment.hpp"

#include <memory>

namespace osc
{
	struct RenderTargetColorAttachment final : public RenderTargetAttachment {

		RenderTargetColorAttachment(
			std::shared_ptr<RenderBuffer>,
			RenderBufferLoadAction,
			RenderBufferStoreAction,
			Color clearColor_
		);

		Color clearColor;
	};

	bool operator==(RenderTargetColorAttachment const&, RenderTargetColorAttachment const&) noexcept;
	bool operator!=(RenderTargetColorAttachment const&, RenderTargetColorAttachment const&) noexcept;
}