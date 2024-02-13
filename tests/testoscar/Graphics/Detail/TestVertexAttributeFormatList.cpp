#include <oscar/Graphics/Detail/VertexAttributeFormatList.h>

#include <gtest/gtest.h>
#include <oscar/Graphics/VertexAttributeFormat.h>
#include <oscar/Graphics/Detail/VertexAttributeFormatTraits.h>
#include <oscar/Utils/EnumHelpers.h>
#include <oscar/Utils/NonTypelist.h>

#include <array>

using namespace osc;
using namespace osc::detail;

TEST(VertexAttributeFormatList, EveryEntryInTheListHasAnAssociatedTraitsObject)
{
    [[maybe_unused]] auto ary = []<VertexAttributeFormat... Formats>(OptionList<VertexAttributeFormat, Formats...>)
    {
        return std::to_array({VertexAttributeFormatTraits<Formats>::num_components...});
    }(VertexAttributeFormatList{});
}
