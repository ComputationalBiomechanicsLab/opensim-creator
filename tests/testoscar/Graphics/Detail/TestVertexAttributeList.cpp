#include <oscar/Graphics/Detail/VertexAttributeList.hpp>

#include <gtest/gtest.h>
#include <oscar/Graphics/Detail/VertexAttributeTraits.hpp>
#include <oscar/Graphics/VertexAttribute.hpp>
#include <oscar/Utils/EnumHelpers.hpp>
#include <oscar/Utils/NonTypelist.hpp>

#include <array>

using osc::detail::VertexAttributeList;
using osc::detail::VertexAttributeTraits;
using osc::NonTypelist;
using osc::NonTypelistSizeV;
using osc::NumOptions;
using osc::VertexAttribute;

namespace
{
    template<VertexAttribute... Attributes>
    constexpr void InstantiateTraits(NonTypelist<VertexAttribute, Attributes...>)
    {
        std::to_array({VertexAttributeTraits<Attributes>::default_format...});
    }
}

TEST(VertexAttributeList, HasAnEntryForEachVertexAttribute)
{
    static_assert(NumOptions<VertexAttribute>() == NonTypelistSizeV<VertexAttributeList>);
}

TEST(VertexAttributeList, EveryEntryInTheListHasAnAssociatedTraitsObject)
{
    InstantiateTraits(VertexAttributeList{});
}
