#pragma once

#include <oscar/Graphics/Detail/VertexAttributeList.hpp>
#include <oscar/Graphics/Detail/VertexAttributeTraits.hpp>
#include <oscar/Graphics/VertexAttribute.hpp>
#include <oscar/Graphics/VertexAttributeFormat.hpp>
#include <oscar/Shims/Cpp23/utility.hpp>
#include <oscar/Utils/NonTypelist.hpp>

#include <array>

namespace osc::detail
{
    constexpr VertexAttributeFormat DefaultFormat(VertexAttribute attr)
    {
        constexpr auto lut = []<VertexAttribute... Attrs>(NonTypelist<VertexAttribute, Attrs...>) {
            return std::to_array({ VertexAttributeTraits<Attrs>::default_format... });
        }(VertexAttributeList{});

        return lut.at(osc::to_underlying(attr));
    }
}
