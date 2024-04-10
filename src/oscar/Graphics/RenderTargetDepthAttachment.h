#pragma once

#include <oscar/Graphics/RenderTargetAttachment.h>

namespace osc
{
    struct RenderTargetDepthAttachment final : public RenderTargetAttachment {
        using RenderTargetAttachment::RenderTargetAttachment;

        friend bool operator==(const RenderTargetDepthAttachment&, const RenderTargetDepthAttachment&) = default;
    };
}
