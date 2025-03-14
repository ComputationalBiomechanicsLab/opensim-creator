#pragma once

#include <liboscar/Graphics/Detail/VertexAttributeFormatList.h>
#include <liboscar/Graphics/Detail/VertexAttributeFormatTraits.h>
#include <liboscar/Graphics/VertexAttributeFormat.h>
#include <liboscar/Utils/EnumHelpers.h>

#include <array>
#include <cstddef>

namespace osc::detail
{
    constexpr size_t stride_of(VertexAttributeFormat format)
    {
        constexpr auto lut = []<VertexAttributeFormat... Formats>(OptionList<VertexAttributeFormat, Formats...>) {
            return std::to_array({ VertexAttributeFormatTraits<Formats>::stride... });
        }(VertexAttributeFormatList{});

        return lut.at(to_index(format));
    }

    constexpr size_t num_components_in(VertexAttributeFormat format)
    {
        constexpr auto lut = []<VertexAttributeFormat... Formats>(OptionList<VertexAttributeFormat, Formats...>) {
            return std::to_array({ VertexAttributeFormatTraits<Formats>::num_components... });
        }(VertexAttributeFormatList{});

        return lut.at(to_index(format));
    }

    constexpr size_t component_size(VertexAttributeFormat format)
    {
        constexpr auto lut = []<VertexAttributeFormat... Formats>(OptionList<VertexAttributeFormat, Formats...>) {
            return std::to_array({ VertexAttributeFormatTraits<Formats>::component_size... });
        }(VertexAttributeFormatList{});

        return lut.at(to_index(format));
    }
}
