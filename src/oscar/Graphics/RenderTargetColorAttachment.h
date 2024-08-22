#pragma once

#include <oscar/Graphics/Color.h>
#include <oscar/Graphics/RenderBufferLoadAction.h>
#include <oscar/Graphics/RenderBufferStoreAction.h>
#include <oscar/Graphics/RenderTargetAttachment.h>
#include <oscar/Graphics/SharedRenderBuffer.h>

#include <utility>

namespace osc
{
    struct RenderTargetColorAttachment final : public RenderTargetAttachment {

        explicit RenderTargetColorAttachment(
            SharedRenderBuffer buffer_,
            RenderBufferLoadAction load_action_,
            RenderBufferStoreAction store_action_,
            Color clear_color_) :

            RenderTargetAttachment{std::move(buffer_), load_action_, store_action_},
            clear_color{clear_color_}
        {}

        friend bool operator==(const RenderTargetColorAttachment&, const RenderTargetColorAttachment&) = default;

        Color clear_color;
    };
}
