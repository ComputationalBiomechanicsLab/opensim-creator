#pragma once

#include <liboscar/Graphics/Color.h>
#include <liboscar/Graphics/RenderBufferLoadAction.h>
#include <liboscar/Graphics/RenderBufferStoreAction.h>
#include <liboscar/Graphics/SharedColorRenderBuffer.h>

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
