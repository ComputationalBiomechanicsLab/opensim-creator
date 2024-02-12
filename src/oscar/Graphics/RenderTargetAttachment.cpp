#include "RenderTargetAttachment.h"

#include <oscar/Utils/Assertions.h>

#include <utility>

osc::RenderTargetAttachment::RenderTargetAttachment(
    std::shared_ptr<RenderBuffer> buffer_,
    RenderBufferLoadAction loadAction_,
    RenderBufferStoreAction storeAction_) :

    buffer{std::move(buffer_)},
    loadAction{loadAction_},
    storeAction{storeAction_}
{
    OSC_ASSERT(buffer != nullptr);
}
