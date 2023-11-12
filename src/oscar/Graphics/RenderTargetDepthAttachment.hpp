#pragma once

#include <oscar/Graphics/RenderTargetAttachment.hpp>

namespace osc
{
    struct RenderTargetDepthAttachment final : public RenderTargetAttachment {
        using RenderTargetAttachment::RenderTargetAttachment;

        friend bool operator==(RenderTargetDepthAttachment const&, RenderTargetDepthAttachment const&) = default;
    };
}
