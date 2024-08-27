#pragma once

#include <oscar/Graphics/DepthStencilRenderBufferFormat.h>
#include <oscar/Utils/EnumHelpers.h>

namespace osc::detail
{
    using DepthStencilRenderBufferFormatList = OptionList<DepthStencilRenderBufferFormat,
        DepthStencilRenderBufferFormat::D24_UNorm_S8_UInt,
        DepthStencilRenderBufferFormat::D32_SFloat
    >;
}
