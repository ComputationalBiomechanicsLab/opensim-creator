#pragma once

#include <oscar/Graphics/RenderBufferLoadAction.h>
#include <oscar/Graphics/RenderBufferStoreAction.h>
#include <oscar/Graphics/SharedDepthStencilRenderBuffer.h>

namespace osc
{
    struct RenderTargetDepthStencilAttachment final {

        friend bool operator==(const RenderTargetDepthStencilAttachment&, const RenderTargetDepthStencilAttachment&) = default;

        SharedDepthStencilRenderBuffer buffer{};
        RenderBufferLoadAction load_action = RenderBufferLoadAction::Clear;
        RenderBufferStoreAction store_action = RenderBufferStoreAction::DontCare;
    };
}
