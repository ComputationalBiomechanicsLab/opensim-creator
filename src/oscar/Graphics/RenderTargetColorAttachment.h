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
            Color clear_color_
        );

        friend bool operator==(const RenderTargetColorAttachment&, const RenderTargetColorAttachment&) = default;

        Color clear_color;
    };
}
