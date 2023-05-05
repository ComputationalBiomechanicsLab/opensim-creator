#pragma once

#include "src/Graphics/RenderBuffer.hpp"
#include "src/Graphics/RenderBufferLoadAction.hpp"
#include "src/Graphics/RenderBufferStoreAction.hpp"

#include <memory>

namespace osc
{
	struct RenderTargetAttachment {
		RenderTargetAttachment(
			std::shared_ptr<RenderBuffer> buffer_,
			RenderBufferLoadAction loadAction_,
			RenderBufferStoreAction storeAction_
		);

		std::shared_ptr<RenderBuffer> buffer;
		RenderBufferLoadAction loadAction;
		RenderBufferStoreAction storeAction;
	};
}