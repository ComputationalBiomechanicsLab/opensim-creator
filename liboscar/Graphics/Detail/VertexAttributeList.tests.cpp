#include "VertexAttributeList.h"

#include <liboscar/Graphics/VertexAttribute.h>
#include <liboscar/Graphics/Detail/VertexAttributeTraits.h>
#include <liboscar/Utils/EnumHelpers.h>

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
