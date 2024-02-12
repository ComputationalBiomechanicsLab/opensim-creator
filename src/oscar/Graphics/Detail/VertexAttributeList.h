#pragma once

#include <oscar/Graphics/VertexAttribute.h>
#include <oscar/Utils/NonTypelist.h>

namespace osc::detail
{
    using VertexAttributeList = NonTypelist<VertexAttribute,
        VertexAttribute::Position,
        VertexAttribute::Normal,
        VertexAttribute::Tangent,
        VertexAttribute::Color,
        VertexAttribute::TexCoord0
    >;
}
