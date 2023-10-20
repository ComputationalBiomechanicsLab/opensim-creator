#include "RenderTargetColorAttachment.hpp"

#include <oscar/Graphics/Color.hpp>
#include <oscar/Graphics/RenderBuffer.hpp>
#include <oscar/Graphics/RenderBufferLoadAction.hpp>
#include <oscar/Graphics/RenderBufferStoreAction.hpp>

#include <memory>
#include <utility>

osc::RenderTargetColorAttachment::RenderTargetColorAttachment(
    std::shared_ptr<RenderBuffer> buffer_,
    RenderBufferLoadAction loadAction_,
    RenderBufferStoreAction storeAction_,
    Color clearColor_) :

    RenderTargetAttachment{std::move(buffer_), loadAction_, storeAction_},
    clearColor{clearColor_}
{
}

bool osc::operator==(RenderTargetColorAttachment const& lhs, RenderTargetColorAttachment const& rhs) noexcept
{
    return
        lhs.buffer == rhs.buffer &&
        lhs.loadAction == rhs.loadAction &&
        lhs.storeAction == rhs.storeAction &&
        lhs.clearColor == rhs.clearColor;
}

bool osc::operator!=(RenderTargetColorAttachment const& lhs, RenderTargetColorAttachment const& rhs) noexcept
{
    return !(lhs == rhs);
}
