#include "VertexAttributeFormatList.h"

#include <liboscar/Graphics/VertexAttributeFormat.h>
#include <liboscar/Graphics/Detail/VertexAttributeFormatTraits.h>
#include <liboscar/Utils/EnumHelpers.h>

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
