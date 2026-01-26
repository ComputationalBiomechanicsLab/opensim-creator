#pragma once

#include <liboscar/graphics/vertex_attribute_format.h>
#include <liboscar/utilities/enum_helpers.h>

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
