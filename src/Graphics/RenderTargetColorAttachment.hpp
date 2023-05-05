#pragma once

#include "src/Graphics/Color.hpp"
#include "src/Graphics/RenderTargetAttachment.hpp"

namespace osc
{
	struct RenderTargetColorAttachment final : public RenderTargetAttachment {
		RenderTargetColorAttachment(
			std::shared_ptr<RenderBuffer> buffer_,
			RenderBufferLoadAction loadAction_,
			RenderBufferStoreAction storeAction_,
			Color clearColor_) :

			RenderTargetAttachment{std::move(buffer_), loadAction_, storeAction_},
			clearColor{clearColor_}
		{
		}

		Color clearColor;
	};
}