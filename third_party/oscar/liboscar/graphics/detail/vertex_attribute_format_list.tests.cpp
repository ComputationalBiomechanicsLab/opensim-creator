#include "vertex_attribute_format_list.h"

#include <liboscar/graphics/vertex_attribute_format.h>
#include <liboscar/graphics/detail/vertex_attribute_format_traits.h>
#include <liboscar/utils/enum_helpers.h>

#include <gtest/gtest.h>

#include <array>

using namespace osc;
using namespace osc::detail;

TEST(VertexAttributeFormatList, every_entry_in_the_list_has_an_associated_VertexAttributeFormatTraits_implementation)
{
    [[maybe_unused]] auto ary = []<VertexAttributeFormat... Formats>(OptionList<VertexAttributeFormat, Formats...>)
    {
        return std::to_array({VertexAttributeFormatTraits<Formats>::num_components...});
    }(VertexAttributeFormatList{});
}
