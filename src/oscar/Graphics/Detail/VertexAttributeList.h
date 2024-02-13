#pragma once

#include <oscar/Graphics/VertexAttribute.h>
#include <oscar/Utils/EnumHelpers.h>

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
