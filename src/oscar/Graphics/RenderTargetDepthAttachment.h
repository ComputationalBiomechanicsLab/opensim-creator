#pragma once

#include <oscar/Graphics/RenderBufferLoadAction.h>
#include <oscar/Graphics/RenderBufferStoreAction.h>
#include <oscar/Graphics/SharedDepthRenderBuffer.h>

namespace osc
{
    struct RenderTargetDepthAttachment final {

        friend bool operator==(const RenderTargetDepthAttachment&, const RenderTargetDepthAttachment&) = default;

        SharedDepthRenderBuffer buffer{};
        RenderBufferLoadAction load_action = RenderBufferLoadAction::Clear;
        RenderBufferStoreAction store_action = RenderBufferStoreAction::DontCare;
    };
}
