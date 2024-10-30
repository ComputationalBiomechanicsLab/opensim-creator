#include <oscar/Utils/StringName.h>

#include <oscar/Utils/CStringView.h>

#include <gtest/gtest.h>

#include <array>
#include <iterator>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

using namespace osc;

namespace
{
    // SSO: Short String Optimization
    //
    // The reason this code is trying to avoid SSO is to increase the chance that a 3rd-party
    // memory analyzer (e.g. libASAN, valgrind) can spot any issues related to allocating strings
    // in `StringName`'s global string table.
    constexpr auto c_long_character_data_array_to_avoid_SSO = std::to_array("somequitelongstringthatprobablyneedstobeheapallocatedsothatmemoryanalyzershaveabetterchance");
    constexpr const char* const c_long_cstring_to_avoid_SSO = c_long_character_data_array_to_avoid_SSO.data();
    constexpr auto c_another_long_character_data_array_to_avoid_SSO = std::to_array("somedifferencequitelongstringthatprobablyneedstobeheapallocatedbutwhoknows");
    constexpr const char* const c_another_long_cstring_to_avoid_SSO = c_another_long_character_data_array_to_avoid_SSO.data();
}

TEST(StringName, can_default_constructs)
{
    ASSERT_NO_THROW({ StringName{}; });
}

TEST(StringName, copy_constructing_default_constructed_instance_compares_equivalent)
{
    const StringName a;
    const StringName b{a};  // NOLINT(performance-unnecessary-copy-initialization)
    ASSERT_EQ(a, b);
}

TEST(StringName, can_move_construct)
{
    StringName a;
    const StringName b{std::move(a)};  // NOLINT(hicpp-move-const-arg,performance-move-const-arg)

    ASSERT_TRUE(b.empty());
    ASSERT_EQ(b.size(), 0);
}

TEST(StringName, copy_assigning_default_constructed_over_non_default_makes_lhs_default)
{
    const StringName a;
    StringName b{c_long_cstring_to_avoid_SSO};
    b = a;
    ASSERT_EQ(a, b);
}

TEST(StringName, MoveAssigningDefaultOverNonDefaultMakesLhsDefault)
{
    StringName a;
    StringName b{c_long_cstring_to_avoid_SSO};
    b = std::move(a);  // NOLINT(hicpp-move-const-arg,performance-move-const-arg)
    ASSERT_EQ(b, StringName{});
}

TEST(StringName, DefaultConstructedReturnsNonNullptrForData)
{
    ASSERT_TRUE(StringName{}.data());
}

TEST(StringName, DefaultConstructedReturnsNonNullptrForCString)
{
    ASSERT_TRUE(StringName{}.c_str());
}

TEST(StringName, DefaultConstructedImplicitlyConvertsIntoBlankStringView)
{
    ASSERT_EQ(static_cast<std::string_view>(StringName{}), std::string_view{});
}

TEST(StringName, DefaultConstructedExplicitlyConvertsIntoCStringView)
{
    ASSERT_EQ(static_cast<CStringView>(StringName{}), CStringView{});
}

TEST(StringName, CanBeUsedToCallCStringViewFunctions)
{
    StringName sn;
    const auto f = [](CStringView) {};
    f(sn);  // should compile
}

TEST(StringName, DefaultConstructedBeginEqualsEnd)
{
    StringName sn;
    ASSERT_EQ(sn.begin(), sn.end());
}

TEST(StringName, DefaultConstructedCBeginEqualsCEnd)
{
    StringName sn;
    ASSERT_EQ(sn.cbegin(), sn.cend());
}

TEST(StringName, DefaultConstructedCBeginEqualsBegin)
{
    StringName sn;
    ASSERT_EQ(sn.begin(), sn.cbegin());
}

TEST(StringName, DefaultConstructedIsEmpty)
{
    ASSERT_TRUE(StringName{}.empty());
}

TEST(StringName, DefaultConstructedSizeIsZero)
{
    ASSERT_EQ(StringName{}.size(), 0);
}

TEST(StringName, DefaultConstructedEqualsAnotherDefaultConstructed)
{
    ASSERT_EQ(StringName{}, StringName{});
}

TEST(StringName, DefaultConstructedCanBeImplicitlyConvertedIntoBlankStringView)
{
    ASSERT_EQ(StringName{}, std::string_view{});
}

TEST(StringName, DefaultConstructedCanBeImplicitlyConvertedIntoBlankCStringView)
{
    ASSERT_EQ(StringName{}, CStringView{});
}

TEST(StringName, DefaultConstructedIsEqualToBlankString)
{
    ASSERT_EQ(StringName{}, std::string{});
}

TEST(StringName, DefaultConstructedIsEqualToBlankStringReversedOp)
{
    ASSERT_EQ(std::string{}, StringName{});
}

TEST(StringName, DefaultConstructedIsEqualToBlankCString)
{
    ASSERT_EQ(StringName{}, "");
}

TEST(StringName, DefaultConstructedIsEqualToBlankCStringReversedOp)
{
    ASSERT_EQ("", StringName{});
}

TEST(StringName, DefaultConstructedIsNotEqualToANonBlankStringName)
{
    ASSERT_NE(StringName{}, StringName{c_long_cstring_to_avoid_SSO});
}

TEST(StringName, DefaultConstructedIsNotEqualToANonBlankStringNameReversedOp)
{
    ASSERT_NE(StringName{c_long_cstring_to_avoid_SSO}, StringName{});
}

TEST(StringName, DefaultConstructedIsNotEqualToANonBlankStringView)
{
    ASSERT_NE(StringName{}, std::string_view{c_long_cstring_to_avoid_SSO});
}

TEST(StringName, DefaultConstructedIsNotEqualToANonBlankStringViewReversedOp)
{
    ASSERT_NE(std::string_view{c_long_cstring_to_avoid_SSO}, StringName{});
}

TEST(StringName, DefaultConstructedIsNotEqualToANonBlankString)
{
    ASSERT_NE(StringName{}, std::string{c_long_cstring_to_avoid_SSO});
}

TEST(StringName, DefaultConstructedIsNotEqualToANonBlankStringReversedOp)
{
    ASSERT_NE(std::string{c_long_cstring_to_avoid_SSO}, StringName{});
}

TEST(StringName, DefaultConstructedIsNotEqualToANonBlankCString)
{
    ASSERT_NE(StringName{}, c_long_cstring_to_avoid_SSO);
}

TEST(StringName, DefaultConstructedIsNotEqualToANonBlankCStringReversedOp)
{
    ASSERT_NE(c_long_cstring_to_avoid_SSO, StringName{});
}

TEST(StringName, DefaultConstructedComparesLessThanContentfulStringView)
{
    ASSERT_LT(StringName{}, StringName{c_long_cstring_to_avoid_SSO});
}

TEST(StringName, DefaultConstructedComparesLessThanContentfulStringViewReversedOp)
{
    ASSERT_LT(StringName{}, StringName{c_long_cstring_to_avoid_SSO});
}

TEST(StringName, DefaultConstructedCanBeStreamedWhichWritesNothingToTheStream)
{
    std::stringstream ss;
    ss << StringName{};
    ASSERT_TRUE(ss.str().empty());
}

TEST(StringName, DefaultConstructedSwappingWorksAsExpectedWithNonEmpty)
{
    StringName a;
    const StringName aCopy{a};
    StringName b{c_long_cstring_to_avoid_SSO};
    const StringName bCopy{b};

    swap(a, b);

    ASSERT_EQ(a, bCopy);
    ASSERT_EQ(b, aCopy);
}

TEST(StringName, DefaultConstructedStringNameHashIsEqualToHashOfBlankString)
{
    ASSERT_EQ(std::hash<StringName>{}(StringName{}), std::hash<std::string>{}(std::string{}));
}

TEST(StringName, DefaultConstructedStringNameHashIsEqualToHashOfBlankStringView)
{
    ASSERT_EQ(std::hash<StringName>{}(StringName{}), std::hash<std::string_view>{}(std::string_view{}));
}

TEST(StringName, CanConstructFromStringView)
{
    ASSERT_NO_THROW({ StringName(std::string_view(c_long_cstring_to_avoid_SSO)); });
}

TEST(StringName, CanConstructFromString)
{
    ASSERT_NO_THROW({ StringName(std::string{c_long_cstring_to_avoid_SSO}); });
}

TEST(StringName, CanConstructFromCString)
{
    ASSERT_NO_THROW({ StringName("somecstring"); });
}

TEST(StringName, CanImplicitlyConstructFromCStringView)
{
    const auto f = [](const CStringView&) {};
    f(CStringView{"cstring"});  // should compile
}

TEST(StringName, CopyAssigningOneNonDefaultConstructedStringNameOverAnotherMakesLhsCompareEqual)
{
    StringName a{c_long_cstring_to_avoid_SSO};
    StringName b{c_another_long_cstring_to_avoid_SSO};
    a = b;
    ASSERT_EQ(a, b);
}

TEST(StringName, MoveAssingingOneDefaultConstructedStringNameOverAnotherMakesLhsCompareEqual)
{
    StringName a{c_long_cstring_to_avoid_SSO};
    StringName b{c_another_long_cstring_to_avoid_SSO};
    StringName bTmp{b};
    a = std::move(bTmp);  // NOLINT(hicpp-move-const-arg,performance-move-const-arg)
    ASSERT_EQ(a, b);
}

TEST(StringName, AtReturnsCharacterAtGivenIndexWithBoundsChecking)
{
    StringName s{"string"};
    ASSERT_EQ(s.at(0), 's');
    ASSERT_EQ(s.at(1), 't');
    ASSERT_EQ(s.at(2), 'r');
    ASSERT_EQ(s.at(3), 'i');
    ASSERT_EQ(s.at(4), 'n');
    ASSERT_EQ(s.at(5), 'g');
    ASSERT_ANY_THROW({ s.at(6); });
}

TEST(StringName, BracketsOperatorReturnsCharacterAtGivenIndex)
{
    StringName s{"string"};
    ASSERT_EQ(s[0], 's');
    ASSERT_EQ(s[1], 't');
    ASSERT_EQ(s[2], 'r');
    ASSERT_EQ(s[3], 'i');
    ASSERT_EQ(s[4], 'n');
    ASSERT_EQ(s[5], 'g');
}

TEST(StringName, FrontReturnsFirstCharacter)
{
    StringName s{"string"};
    ASSERT_EQ(s.front(), 's');
}

TEST(StringName, BackReturnsLastCharacter)
{
    StringName s{"string"};
    ASSERT_EQ(s.back(), 'g');
}

TEST(StringName, DataReturnsNulTerminatedPointerToFirstElement)
{
    constexpr auto c_Input = std::to_array("string");
    StringName s{c_Input.data()};

    std::span<const char> inputSpan(c_Input);
    std::span<const StringName::value_type> outputSpan{s.data(), std::size(c_Input)};  // plus nul terminator

    ASSERT_TRUE(std::equal(outputSpan.begin(), outputSpan.end(), inputSpan.begin(), inputSpan.end()));
}

TEST(StringName, CStringReturnsNulTerminatedPointerToFirstElement)
{
    constexpr auto c_Input = std::to_array("string");
    StringName s{c_Input.data()};

    std::span<const char> inputSpan(c_Input);
    std::span<const StringName::value_type> outputSpan{s.c_str(), std::size(c_Input)};  // plus nul terminator

    ASSERT_TRUE(std::equal(outputSpan.begin(), outputSpan.end(), inputSpan.begin(), inputSpan.end()));
}

TEST(StringName, ImplicitlyConvertingToStringViewWorksAsExpected)
{
    const StringName s{c_long_cstring_to_avoid_SSO};
    ASSERT_EQ(static_cast<std::string_view>(s), std::string_view{c_long_cstring_to_avoid_SSO});
}

TEST(StringName, ImplicitlyConvertingToCStringViewWorksAsExpected)
{
    const StringName s{c_long_cstring_to_avoid_SSO};
    ASSERT_EQ(static_cast<CStringView>(s), CStringView{c_long_cstring_to_avoid_SSO});
}

TEST(StringName, BeginNotEqualToEndForNonEmptyString)
{
    StringName s{c_long_cstring_to_avoid_SSO};
    ASSERT_NE(s.begin(), s.end());
}

TEST(StringName, CBeginNotEqualToCendForNonEmptyString)
{
    StringName s{c_long_cstring_to_avoid_SSO};
    ASSERT_NE(s.cbegin(), s.cend());
}

TEST(StringName, BeginIsEqualToCBeginForNonEmptyString)
{
    StringName s{c_long_cstring_to_avoid_SSO};
    ASSERT_EQ(s.begin(), s.cbegin());
}

TEST(StringName, EndIsEqualToCendForNonEmptyString)
{
    StringName s{c_long_cstring_to_avoid_SSO};
    ASSERT_EQ(s.end(), s.cend());
}

TEST(StringName, EmptyReturnsFalseForNonEmptyString)
{
    StringName s{c_long_cstring_to_avoid_SSO};
    ASSERT_FALSE(s.empty());
}

TEST(StringName, SizeReturnsExpectedValue)
{
    StringName s{c_long_cstring_to_avoid_SSO};
    ASSERT_EQ(s.size(), c_long_character_data_array_to_avoid_SSO.size()-1);  // minus nul
}

TEST(StringName, NonEmptyStringNameComapresEqualToAnotherLogicallyEquivalentNonEmptyStringName)
{
    ASSERT_EQ(StringName{c_long_cstring_to_avoid_SSO}, StringName{c_long_cstring_to_avoid_SSO});
}

TEST(StringName, NonEmptyStringNameComparesEquivalentToStringView)
{
    ASSERT_EQ(StringName{c_long_cstring_to_avoid_SSO}, std::string_view{c_long_cstring_to_avoid_SSO});
}

TEST(StringName, NonEmptyStringNameComparesEquivalentToStringViewReversedOp)
{
    ASSERT_EQ(std::string_view{c_long_cstring_to_avoid_SSO}, StringName{c_long_cstring_to_avoid_SSO});
}

TEST(StringName, NonEmptyStringNameComparesEquivalentToCString)
{
    ASSERT_EQ(StringName{c_long_cstring_to_avoid_SSO}, c_long_cstring_to_avoid_SSO);
}

TEST(StringName, NonEmptyStringNameComparesEquivalentToCStringReversedOp)
{
    ASSERT_EQ(c_long_cstring_to_avoid_SSO, StringName{c_long_cstring_to_avoid_SSO});
}

TEST(StringName, NonEmptyStringNameComaparesEquivalentToCStringView)
{
    ASSERT_EQ(StringName{c_long_cstring_to_avoid_SSO}, CStringView{c_long_cstring_to_avoid_SSO});
}

TEST(StringName, NonEmptyStringNameComaparesEquivalentToCStringViewReversedOp)
{
    ASSERT_EQ(CStringView{c_long_cstring_to_avoid_SSO}, StringName{c_long_cstring_to_avoid_SSO});
}

TEST(StringName, ComparesNotEqualToInequivalentString)
{
    ASSERT_NE(StringName{c_long_cstring_to_avoid_SSO}, StringName{c_another_long_cstring_to_avoid_SSO});
}

TEST(StringName, ComparesNotEqualToInequivalentStringReversedOp)
{
    ASSERT_NE(StringName{c_another_long_cstring_to_avoid_SSO}, StringName{c_long_cstring_to_avoid_SSO});
}

TEST(StringName, StreamsCorrectlyToOutputString)
{
    std::stringstream ss;
    ss << StringName{c_long_cstring_to_avoid_SSO};
    ASSERT_EQ(ss.str(), c_long_cstring_to_avoid_SSO);
}

TEST(StringName, NonEmptyStringNameHasSameHashAsEquivalentStringName)
{
    ASSERT_EQ(std::hash<StringName>{}(StringName{c_long_cstring_to_avoid_SSO}), std::hash<StringName>{}(StringName{c_long_cstring_to_avoid_SSO}));
}

TEST(StringName, NonEmptyStringNameHasSameHashAsEquivalentString)
{
    ASSERT_EQ(std::hash<StringName>{}(StringName{c_long_cstring_to_avoid_SSO}), std::hash<std::string>{}(std::string{c_long_cstring_to_avoid_SSO}));
}

TEST(StringName, NonEmptyStringNameHasSameHashAsEquivalentStringView)
{
    ASSERT_EQ(std::hash<StringName>{}(StringName{c_long_cstring_to_avoid_SSO}), std::hash<std::string_view>{}(std::string_view{c_long_cstring_to_avoid_SSO}));
}

TEST(StringName, CanBeStreamedToOStreamAndProducesIdenticalOutputToString)
{
    std::string str{"some streamed string"};
    std::stringstream strss;
    strss << str;
    std::stringstream snss;
    snss << StringName{str};

    ASSERT_EQ(strss.str(), snss.str());
}

