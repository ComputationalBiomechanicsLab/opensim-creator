#pragma once

#include <oscar/Graphics/Detail/VertexAttributeList.h>
#include <oscar/Graphics/Detail/VertexAttributeTraits.h>
#include <oscar/Graphics/VertexAttribute.h>
#include <oscar/Graphics/VertexAttributeFormat.h>
#include <oscar/Utils/EnumHelpers.h>

#include <array>

namespace osc::detail
{
    constexpr VertexAttributeFormat DefaultFormat(VertexAttribute attr)
    {
        constexpr auto lut = []<VertexAttribute... Attrs>(OptionList<VertexAttribute, Attrs...>) {
            return std::to_array({ VertexAttributeTraits<Attrs>::default_format... });
        }(VertexAttributeList{});

        return lut.at(ToIndex(attr));
    }
}
