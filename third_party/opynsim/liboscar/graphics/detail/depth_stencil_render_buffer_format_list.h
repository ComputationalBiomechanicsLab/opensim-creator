#pragma once

#include <liboscar/graphics/depth_stencil_render_buffer_format.h>
#include <liboscar/utilities/enum_helpers.h>

namespace osc::detail
{
    using DepthStencilRenderBufferFormatList = OptionList<DepthStencilRenderBufferFormat,
        DepthStencilRenderBufferFormat::D24_UNorm_S8_UInt,
        DepthStencilRenderBufferFormat::D32_SFloat
    >;
}
