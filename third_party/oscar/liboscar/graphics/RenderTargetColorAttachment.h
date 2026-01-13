#pragma once

#include <liboscar/graphics/Color.h>
#include <liboscar/graphics/RenderBufferLoadAction.h>
#include <liboscar/graphics/RenderBufferStoreAction.h>
#include <liboscar/graphics/SharedColorRenderBuffer.h>

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
