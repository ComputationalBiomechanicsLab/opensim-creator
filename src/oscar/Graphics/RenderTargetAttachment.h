#pragma once

#include <oscar/Graphics/RenderBuffer.h>
#include <oscar/Graphics/RenderBufferLoadAction.h>
#include <oscar/Graphics/RenderBufferStoreAction.h>

#include <memory>

namespace osc
{
    struct RenderTargetAttachment {
        RenderTargetAttachment(
            std::shared_ptr<RenderBuffer> buffer_,
            RenderBufferLoadAction loadAction_,
            RenderBufferStoreAction storeAction_
        );

        friend bool operator==(RenderTargetAttachment const&, RenderTargetAttachment const&) = default;

        std::shared_ptr<RenderBuffer> buffer;
        RenderBufferLoadAction loadAction;
        RenderBufferStoreAction storeAction;
    };
}
