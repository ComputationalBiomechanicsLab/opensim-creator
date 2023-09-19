#pragma once

#include <oscar/Graphics/RenderBuffer.hpp>
#include <oscar/Graphics/RenderBufferLoadAction.hpp>
#include <oscar/Graphics/RenderBufferStoreAction.hpp>

#include <memory>

namespace osc
{
    struct RenderTargetAttachment {
        RenderTargetAttachment(
            std::shared_ptr<RenderBuffer> buffer_,
            RenderBufferLoadAction loadAction_,
            RenderBufferStoreAction storeAction_
        );

        std::shared_ptr<RenderBuffer> buffer;
        RenderBufferLoadAction loadAction;
        RenderBufferStoreAction storeAction;
    };
}
