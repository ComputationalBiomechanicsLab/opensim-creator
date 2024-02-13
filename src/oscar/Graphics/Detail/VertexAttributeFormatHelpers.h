#pragma once

#include <oscar/Graphics/Detail/VertexAttributeFormatList.h>
#include <oscar/Graphics/Detail/VertexAttributeFormatTraits.h>
#include <oscar/Graphics/VertexAttributeFormat.h>
#include <oscar/Utils/EnumHelpers.h>

#include <array>
#include <cstddef>

namespace osc::detail
{
    constexpr size_t StrideOf(VertexAttributeFormat f)
    {
        constexpr auto lut = []<VertexAttributeFormat... Formats>(OptionList<VertexAttributeFormat, Formats...>) {
            return std::to_array({ VertexAttributeFormatTraits<Formats>::stride... });
        }(VertexAttributeFormatList{});

        return lut.at(ToIndex(f));
    }

    constexpr size_t NumComponents(VertexAttributeFormat f)
    {
        constexpr auto lut = []<VertexAttributeFormat... Formats>(OptionList<VertexAttributeFormat, Formats...>) {
            return std::to_array({ VertexAttributeFormatTraits<Formats>::num_components... });
        }(VertexAttributeFormatList{});

        return lut.at(ToIndex(f));
    }

    constexpr size_t SizeOfComponent(VertexAttributeFormat f)
    {
        constexpr auto lut = []<VertexAttributeFormat... Formats>(OptionList<VertexAttributeFormat, Formats...>) {
            return std::to_array({ VertexAttributeFormatTraits<Formats>::component_size... });
        }(VertexAttributeFormatList{});

        return lut.at(ToIndex(f));
    }
}
