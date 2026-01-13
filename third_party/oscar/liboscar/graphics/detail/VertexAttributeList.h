#pragma once

#include <liboscar/graphics/VertexAttribute.h>
#include <liboscar/utils/EnumHelpers.h>

namespace osc::detail
{
    using VertexAttributeList = OptionList<VertexAttribute,
        VertexAttribute::Position,
        VertexAttribute::Normal,
        VertexAttribute::Tangent,
        VertexAttribute::Color,
        VertexAttribute::TexCoord0
    >;
}
