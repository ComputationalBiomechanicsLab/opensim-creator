#pragma once

#include <oscar/Graphics/RenderBufferLoadAction.h>
#include <oscar/Graphics/RenderBufferStoreAction.h>
#include <oscar/Graphics/SharedRenderBuffer.h>

namespace osc
{
    struct RenderTargetAttachment {

        explicit RenderTargetAttachment(
            SharedRenderBuffer buffer_,
            RenderBufferLoadAction load_action_,
            RenderBufferStoreAction store_action_) :

            buffer{std::move(buffer_)},
            load_action{load_action_},
            store_action{store_action_}
        {}

        friend bool operator==(const RenderTargetAttachment&, const RenderTargetAttachment&) = default;

        SharedRenderBuffer buffer;
        RenderBufferLoadAction load_action;
        RenderBufferStoreAction store_action;
    };
}
