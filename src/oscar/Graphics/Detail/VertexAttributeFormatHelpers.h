#pragma once

#include <oscar/Graphics/Detail/VertexAttributeFormatList.h>
#include <oscar/Graphics/Detail/VertexAttributeFormatTraits.h>
#include <oscar/Graphics/VertexAttributeFormat.h>
#include <oscar/Utils/EnumHelpers.h>

#include <array>
#include <cstddef>

namespace osc::detail
{
    constexpr size_t stride_of(VertexAttributeFormat f)
    {
        constexpr auto lut = []<VertexAttributeFormat... Formats>(OptionList<VertexAttributeFormat, Formats...>) {
            return std::to_array({ VertexAttributeFormatTraits<Formats>::stride... });
        }(VertexAttributeFormatList{});

        return lut.at(to_index(f));
    }

    constexpr size_t num_components_in(VertexAttributeFormat f)
    {
        constexpr auto lut = []<VertexAttributeFormat... Formats>(OptionList<VertexAttributeFormat, Formats...>) {
            return std::to_array({ VertexAttributeFormatTraits<Formats>::num_components... });
        }(VertexAttributeFormatList{});

        return lut.at(to_index(f));
    }

    constexpr size_t component_size(VertexAttributeFormat f)
    {
        constexpr auto lut = []<VertexAttributeFormat... Formats>(OptionList<VertexAttributeFormat, Formats...>) {
            return std::to_array({ VertexAttributeFormatTraits<Formats>::component_size... });
        }(VertexAttributeFormatList{});

        return lut.at(to_index(f));
    }
}
