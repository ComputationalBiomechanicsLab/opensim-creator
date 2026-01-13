#pragma once

#include <liboscar/graphics/RenderBufferLoadAction.h>
#include <liboscar/graphics/RenderBufferStoreAction.h>
#include <liboscar/graphics/SharedDepthStencilRenderBuffer.h>

namespace osc
{
    struct RenderTargetDepthStencilAttachment final {

        friend bool operator==(const RenderTargetDepthStencilAttachment&, const RenderTargetDepthStencilAttachment&) = default;

        SharedDepthStencilRenderBuffer buffer{};
        RenderBufferLoadAction load_action = RenderBufferLoadAction::Clear;
        RenderBufferStoreAction store_action = RenderBufferStoreAction::DontCare;
    };
}
