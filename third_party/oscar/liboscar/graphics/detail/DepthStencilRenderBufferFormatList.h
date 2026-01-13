#pragma once

#include <liboscar/graphics/DepthStencilRenderBufferFormat.h>
#include <liboscar/utils/EnumHelpers.h>

namespace osc::detail
{
    using DepthStencilRenderBufferFormatList = OptionList<DepthStencilRenderBufferFormat,
        DepthStencilRenderBufferFormat::D24_UNorm_S8_UInt,
        DepthStencilRenderBufferFormat::D32_SFloat
    >;
}
