#pragma once

#include "oscar/Graphics/Color.hpp"
#include "oscar/Graphics/RenderBufferLoadAction.hpp"
#include "oscar/Graphics/RenderBufferStoreAction.hpp"
#include "oscar/Graphics/RenderTargetAttachment.hpp"

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