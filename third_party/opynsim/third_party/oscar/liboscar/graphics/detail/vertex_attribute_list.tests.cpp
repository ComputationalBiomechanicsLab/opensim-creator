#include "vertex_attribute_list.h"

#include <liboscar/graphics/vertex_attribute.h>
#include <liboscar/graphics/detail/vertex_attribute_traits.h>
#include <liboscar/utils/enum_helpers.h>

#include <gtest/gtest.h>

#include <array>

using namespace osc;
using namespace osc::detail;

TEST(VertexAttributeList, every_entry_has_an_associted_VertexAttributeTraits_implementation)
{
    [[maybe_unused]] constexpr auto ary = []<VertexAttribute... Attributes>(OptionList<VertexAttribute, Attributes...>)
    {
        return std::to_array({VertexAttributeTraits<Attributes>::default_format...});
    }(VertexAttributeList{});
}
