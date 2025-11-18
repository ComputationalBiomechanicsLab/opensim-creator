#pragma once

#include <liboscar/Graphics/RenderBufferLoadAction.h>
#include <liboscar/Graphics/RenderBufferStoreAction.h>
#include <liboscar/Graphics/SharedDepthStencilRenderBuffer.h>

namespace osc
{
    struct RenderTargetDepthStencilAttachment final {

        friend bool operator==(const RenderTargetDepthStencilAttachment&, const RenderTargetDepthStencilAttachment&) = default;

        SharedDepthStencilRenderBuffer buffer{};
        RenderBufferLoadAction load_action = RenderBufferLoadAction::Clear;
        RenderBufferStoreAction store_action = RenderBufferStoreAction::DontCare;
    };
}
