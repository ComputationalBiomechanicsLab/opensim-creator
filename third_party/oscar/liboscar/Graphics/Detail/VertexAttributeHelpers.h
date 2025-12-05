#pragma once

#include <liboscar/Graphics/Detail/VertexAttributeList.h>
#include <liboscar/Graphics/Detail/VertexAttributeTraits.h>
#include <liboscar/Graphics/VertexAttribute.h>
#include <liboscar/Graphics/VertexAttributeFormat.h>
#include <liboscar/Utils/EnumHelpers.h>

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
