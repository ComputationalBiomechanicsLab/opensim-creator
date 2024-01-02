#pragma once

#include <oscar/Graphics/VertexAttribute.hpp>
#include <oscar/Utils/NonTypelist.hpp>

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
