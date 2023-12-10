#pragma once

#include <oscar/Graphics/VertexAttribute.hpp>
#include <oscar/Graphics/VertexAttributeFormat.hpp>

#include <cstddef>

namespace osc
{
    struct VertexAttributeDescriptor final {

        VertexAttributeDescriptor(
            VertexAttribute attribute_,
            VertexAttributeFormat format_,
            size_t dimension_) :

            attribute{attribute_},
            format{format_},
            dimension{dimension_}
        {
        }

        VertexAttribute attribute;
        VertexAttributeFormat format;
        size_t dimension;
    };
}
