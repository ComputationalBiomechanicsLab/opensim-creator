#pragma once

#include <liboscar/Graphics/VertexAttributeFormat.h>
#include <liboscar/Utils/EnumHelpers.h>

namespace osc::detail
{
    using VertexAttributeFormatList = OptionList<VertexAttributeFormat,
        VertexAttributeFormat::Float32x2,
        VertexAttributeFormat::Float32x3,
        VertexAttributeFormat::Float32x4,
        VertexAttributeFormat::Unorm8x4,
        VertexAttributeFormat::Snorm8x4
    >;
}
