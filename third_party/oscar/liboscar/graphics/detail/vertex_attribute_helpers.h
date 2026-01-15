#pragma once

#include <liboscar/graphics/detail/vertex_attribute_list.h>
#include <liboscar/graphics/detail/vertex_attribute_traits.h>
#include <liboscar/graphics/vertex_attribute.h>
#include <liboscar/graphics/vertex_attribute_format.h>
#include <liboscar/utils/enum_helpers.h>

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
