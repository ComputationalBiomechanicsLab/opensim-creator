#include "RenderTargetAttachment.hpp"

#include "oscar/Utils/Assertions.hpp"

#include <utility>

osc::RenderTargetAttachment::RenderTargetAttachment(
    std::shared_ptr<RenderBuffer> buffer_,
    RenderBufferLoadAction loadAction_,
    RenderBufferStoreAction storeAction_) :

    buffer{std::move(buffer_)},
    loadAction{loadAction_},
    storeAction{storeAction_}
{
    OSC_THROWING_ASSERT(buffer != nullptr);
}
