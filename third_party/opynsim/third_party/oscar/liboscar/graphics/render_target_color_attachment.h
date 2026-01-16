#pragma once

#include <liboscar/graphics/color.h>
#include <liboscar/graphics/render_buffer_load_action.h>
#include <liboscar/graphics/render_buffer_store_action.h>
#include <liboscar/graphics/shared_color_render_buffer.h>

namespace osc
{
    struct RenderTargetColorAttachment final {

        friend bool operator==(const RenderTargetColorAttachment&, const RenderTargetColorAttachment&) = default;

        SharedColorRenderBuffer buffer{};
        RenderBufferLoadAction load_action = RenderBufferLoadAction::Clear;
        RenderBufferStoreAction store_action = RenderBufferStoreAction::Resolve;
        Color clear_color = Color::clear();
    };
}
