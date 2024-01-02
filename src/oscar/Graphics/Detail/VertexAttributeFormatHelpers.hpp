#pragma once

#include <oscar/Graphics/Detail/VertexAttributeFormatList.hpp>
#include <oscar/Graphics/Detail/VertexAttributeFormatTraits.hpp>
#include <oscar/Graphics/VertexAttributeFormat.hpp>
#include <oscar/Shims/Cpp23/utility.hpp>
#include <oscar/Utils/NonTypelist.hpp>

#include <array>
#include <cstddef>

namespace osc::detail
{
    template<VertexAttributeFormat... Formats>
    constexpr size_t LookupStride(NonTypelist<VertexAttributeFormat, Formats...>, VertexAttributeFormat f)
    {
        auto const els = std::to_array({ VertexAttributeFormatTraits<Formats>::stride... });
        return els.at(osc::to_underlying(f));
    }

    constexpr size_t StrideOf(VertexAttributeFormat f)
    {
        return LookupStride(VertexAttributeFormatList{}, f);
    }

    template<VertexAttributeFormat... Formats>
    constexpr size_t LookupNumComponents(NonTypelist<VertexAttributeFormat, Formats...>, VertexAttributeFormat f)
    {
        auto const els = std::to_array({ VertexAttributeFormatTraits<Formats>::num_components... });
        return els.at(osc::to_underlying(f));
    }

    constexpr size_t NumComponents(VertexAttributeFormat f)
    {
        return LookupNumComponents(VertexAttributeFormatList{}, f);
    }

    template<VertexAttributeFormat... Formats>
    constexpr size_t LookupSizeOfComponent(NonTypelist<VertexAttributeFormat, Formats...>, VertexAttributeFormat f)
    {
        auto const els = std::to_array({ VertexAttributeFormatTraits<Formats>::component_size... });
        return els.at(osc::to_underlying(f));
    }

    constexpr size_t SizeOfComponent(VertexAttributeFormat f)
    {
        return LookupSizeOfComponent(VertexAttributeFormatList{}, f);
    }
}
