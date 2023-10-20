#pragma once

#include <oscar/Graphics/Color.hpp>
#include <oscar/Graphics/RenderBufferLoadAction.hpp>
#include <oscar/Graphics/RenderBufferStoreAction.hpp>
#include <oscar/Graphics/RenderTargetAttachment.hpp>

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

        friend bool operator==(RenderTargetColorAttachment const&, RenderTargetColorAttachment const&) noexcept;
        friend bool operator!=(RenderTargetColorAttachment const&, RenderTargetColorAttachment const&) noexcept;

        Color clearColor;
    };
}
