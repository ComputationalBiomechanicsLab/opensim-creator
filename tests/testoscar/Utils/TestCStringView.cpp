#include <oscar/Utils/CStringView.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <string_view>

using namespace osc;
using namespace osc::literals;

TEST(CStringView, WhenPassedNullCstringYieldsEmptyCStringView)
{
    const char* p = nullptr;
    ASSERT_TRUE(CStringView{p}.empty());
}

TEST(CStringView, WhenPassedNullCStringYieldsNonNullCStr)
{
    const char* p = nullptr;
    ASSERT_NE(CStringView{p}.c_str(), nullptr);
}

TEST(CStringView, WhenDefaultConstructedYieldsEmptyCStringView)
{
    ASSERT_TRUE(CStringView{}.empty());
}

TEST(CStringView, WhenDefaultConstructedYieldsNonNullCStr)
{
    ASSERT_NE(CStringView{}.c_str(), nullptr);
}

TEST(CStringView, WhenConstructedFromNullptrYieldsEmptyCStringView)
{
    ASSERT_TRUE(CStringView{nullptr}.empty());
}

TEST(CStringView, WhenConstructedFromNullptrYieldsNonNullCStr)
{
    ASSERT_NE(CStringView{nullptr}.c_str(), nullptr);
}

TEST(CStringView, ThreeWayComparisonBehavesIdenticallyToStringViewComparision)
{
    const auto svs = std::to_array<const char*>({ "x", "somestring", "somethingelse", "", "_i hope it works ;)" });
    auto sameThreeWayResultWithAllOtherElements = [&svs](const char* elCStr)
    {
        std::string_view sv{elCStr};
        CStringView csv{elCStr};
        for (const char* otherCStr : svs) {
            ASSERT_EQ(sv <=> std::string_view{otherCStr}, csv <=> CStringView{otherCStr});
        }
    };
    std::for_each(svs.begin(), svs.end(), sameThreeWayResultWithAllOtherElements);
}

TEST(CStringView, LiteralSuffixReturnsCStringView)
{
    static_assert(std::same_as<decltype("hello"_cs), CStringView>);
    ASSERT_EQ("hello"_cs, CStringView{"hello"});
}
