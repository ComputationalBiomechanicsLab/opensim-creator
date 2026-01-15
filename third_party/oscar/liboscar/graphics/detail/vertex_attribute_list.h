#pragma once

#include <liboscar/graphics/vertex_attribute.h>
#include <liboscar/utils/enum_helpers.h>

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
