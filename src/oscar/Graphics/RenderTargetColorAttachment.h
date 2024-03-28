#pragma once

#include <oscar/Graphics/Color.h>
#include <oscar/Graphics/RenderBufferLoadAction.h>
#include <oscar/Graphics/RenderBufferStoreAction.h>
#include <oscar/Graphics/RenderTargetAttachment.h>

#include <memory>

namespace osc
{
    struct RenderTargetColorAttachment final : public RenderTargetAttachment {

        RenderTargetColorAttachment(
            std::shared_ptr<RenderBuffer>,
            RenderBufferLoadAction,
            RenderBufferStoreAction,
            Color clearColor_
        );

        friend bool operator==(RenderTargetColorAttachment const&, RenderTargetColorAttachment const&) = default;

        Color clear_color;
    };
}
