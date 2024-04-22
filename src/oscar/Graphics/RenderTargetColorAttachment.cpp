#include "RenderTargetColorAttachment.h"

#include <oscar/Graphics/Color.h>
#include <oscar/Graphics/RenderBuffer.h>
#include <oscar/Graphics/RenderBufferLoadAction.h>
#include <oscar/Graphics/RenderBufferStoreAction.h>

#include <memory>
#include <utility>

osc::RenderTargetColorAttachment::RenderTargetColorAttachment(
    std::shared_ptr<RenderBuffer> buffer_,
    RenderBufferLoadAction loadAction_,
    RenderBufferStoreAction storeAction_,
    Color clear_color_) :

    RenderTargetAttachment{std::move(buffer_), loadAction_, storeAction_},
    clear_color{clear_color_}
{}
