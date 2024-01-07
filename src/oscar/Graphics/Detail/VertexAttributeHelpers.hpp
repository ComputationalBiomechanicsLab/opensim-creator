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
    template<VertexAttribute... Attrs>
    constexpr VertexAttributeFormat LookupDefaultFormat(NonTypelist<VertexAttribute, Attrs...>, VertexAttribute attr)
    {
        auto els = std::to_array({ VertexAttributeTraits<Attrs>::default_format... });
        return els.at(osc::to_underlying(attr));
    }

    constexpr VertexAttributeFormat DefaultFormat(VertexAttribute attr)
    {
        return LookupDefaultFormat(VertexAttributeList{}, attr);
    }
}
