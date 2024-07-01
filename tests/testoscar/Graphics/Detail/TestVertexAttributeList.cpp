#include <oscar/Graphics/Detail/VertexAttributeList.h>

#include <gtest/gtest.h>
#include <oscar/Graphics/VertexAttribute.h>
#include <oscar/Graphics/Detail/VertexAttributeTraits.h>
#include <oscar/Utils/EnumHelpers.h>

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
