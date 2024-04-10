#pragma once

#include <oscar/Graphics/RenderBuffer.h>
#include <oscar/Graphics/RenderBufferLoadAction.h>
#include <oscar/Graphics/RenderBufferStoreAction.h>

#include <memory>

namespace osc
{
    struct RenderTargetAttachment {
        RenderTargetAttachment(
            std::shared_ptr<RenderBuffer>,
            RenderBufferLoadAction,
            RenderBufferStoreAction
        );

        friend bool operator==(const RenderTargetAttachment&, const RenderTargetAttachment&) = default;

        std::shared_ptr<RenderBuffer> buffer;
        RenderBufferLoadAction load_action;
        RenderBufferStoreAction store_action;
    };
}
