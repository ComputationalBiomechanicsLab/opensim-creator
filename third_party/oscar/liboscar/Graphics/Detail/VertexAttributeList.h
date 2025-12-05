#pragma once

#include <liboscar/Graphics/VertexAttribute.h>
#include <liboscar/Utils/EnumHelpers.h>

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
