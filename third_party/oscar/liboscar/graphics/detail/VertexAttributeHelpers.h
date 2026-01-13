#pragma once

#include <liboscar/graphics/detail/VertexAttributeList.h>
#include <liboscar/graphics/detail/VertexAttributeTraits.h>
#include <liboscar/graphics/VertexAttribute.h>
#include <liboscar/graphics/VertexAttributeFormat.h>
#include <liboscar/utils/EnumHelpers.h>

#include <array>

namespace osc::detail
{
    constexpr VertexAttributeFormat default_format(VertexAttribute attribute)
    {
        constexpr auto lut = []<VertexAttribute... Attrs>(OptionList<VertexAttribute, Attrs...>) {
            return std::to_array({ VertexAttributeTraits<Attrs>::default_format... });
        }(VertexAttributeList{});

        return lut.at(to_index(attribute));
    }
}
