#pragma once

#include <liboscar/Graphics/DepthStencilRenderBufferFormat.h>
#include <liboscar/Utils/EnumHelpers.h>

namespace osc::detail
{
    using DepthStencilRenderBufferFormatList = OptionList<DepthStencilRenderBufferFormat,
        DepthStencilRenderBufferFormat::D24_UNorm_S8_UInt,
        DepthStencilRenderBufferFormat::D32_SFloat
    >;
}
