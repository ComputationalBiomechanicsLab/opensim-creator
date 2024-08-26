#pragma once

#include <oscar/Graphics/Color.h>
#include <oscar/Graphics/RenderBufferLoadAction.h>
#include <oscar/Graphics/RenderBufferStoreAction.h>
#include <oscar/Graphics/SharedColorRenderBuffer.h>

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
