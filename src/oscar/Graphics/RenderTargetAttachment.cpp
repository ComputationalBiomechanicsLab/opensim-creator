#include "RenderTargetAttachment.h"

#include <oscar/Utils/Assertions.h>

#include <utility>

osc::RenderTargetAttachment::RenderTargetAttachment(
    std::shared_ptr<RenderBuffer> buffer_,
    RenderBufferLoadAction load_action_,
    RenderBufferStoreAction store_action_) :

    buffer{std::move(buffer_)},
    load_action{load_action_},
    store_action{store_action_}
{
    OSC_ASSERT(buffer != nullptr);
}
