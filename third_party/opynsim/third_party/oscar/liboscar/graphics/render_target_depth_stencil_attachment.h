#pragma once

#include <liboscar/graphics/render_buffer_load_action.h>
#include <liboscar/graphics/render_buffer_store_action.h>
#include <liboscar/graphics/shared_depth_stencil_render_buffer.h>

namespace osc
{
    struct RenderTargetDepthStencilAttachment final {

        friend bool operator==(const RenderTargetDepthStencilAttachment&, const RenderTargetDepthStencilAttachment&) = default;

        SharedDepthStencilRenderBuffer buffer{};
        RenderBufferLoadAction load_action = RenderBufferLoadAction::Clear;
        RenderBufferStoreAction store_action = RenderBufferStoreAction::DontCare;
    };
}
