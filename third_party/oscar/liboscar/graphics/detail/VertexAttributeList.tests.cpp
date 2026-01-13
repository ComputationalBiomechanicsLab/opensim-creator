#include "VertexAttributeList.h"

#include <liboscar/graphics/VertexAttribute.h>
#include <liboscar/graphics/detail/VertexAttributeTraits.h>
#include <liboscar/utils/EnumHelpers.h>

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
