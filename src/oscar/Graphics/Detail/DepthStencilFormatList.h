#pragma once

#include <oscar/Graphics/DepthStencilFormat.h>
#include <oscar/Utils/EnumHelpers.h>

namespace osc::detail
{
    using DepthStencilFormatList = OptionList<DepthStencilFormat,
        DepthStencilFormat::D24_UNorm_S8_UInt,
        DepthStencilFormat::D32_SFloat
    >;
}
