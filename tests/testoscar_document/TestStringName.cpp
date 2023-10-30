#include <oscar_document/StringName.hpp>

#include <oscar/Utils/Cpp20Shims.hpp>
#include <oscar/Utils/CStringView.hpp>

#include <gtest/gtest.h>
#include <nonstd/span.hpp>

#include <iterator>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

using osc::CStringView;
using osc::StringName;

constexpr auto c_LongStringToAvoidSSOData = osc::to_array("somequitelongstringthatprobablyneedstobeheapallocatedsothatmemoryanalyzershaveabetterchance");
constexpr char const* const c_LongStringToAvoidSSO = c_LongStringToAvoidSSOData.data();
constexpr auto c_AnotherStringToAvoidSSOData = osc::to_array("somedifferencequitelongstringthatprobablyneedstobeheapallocatedbutwhoknows");
constexpr char const* const c_AnotherStringToAvoidSSO = c_AnotherStringToAvoidSSOData.data();

TEST(StringName, CanBeDefaultConstructed)
{
    ASSERT_NO_THROW({ StringName{}; });
}

TEST(StringName, CopyConstructingFromDefaultConstructedComparesEqualToDefaultConstructed)
{
    StringName a;
    StringName b{a};
    ASSERT_EQ(a, b);
}

TEST(StringName, MoveConstructedWorksAsExpected)
{
    StringName a;
    StringName b{std::move(a)};

    ASSERT_TRUE(b.empty());
    ASSERT_EQ(b.size(), 0);
}

TEST(StringName, CopyAssigningDefaultOverNonDefaultMakesLhsDefault)
{
    StringName a;
    StringName b{c_LongStringToAvoidSSO};
    b = a;
    ASSERT_EQ(a, b);
}

TEST(StringName, MoveAssigningDefaultOverNonDefaultMakesLhsDefault)
{
    StringName a;
    StringName b{c_LongStringToAvoidSSO};
    b = std::move(a);
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

TEST(StringName, DefaultConstructedImplicitlyConvertsIntoBlankCStringView)
{
    ASSERT_EQ(static_cast<CStringView>(StringName{}), CStringView{});
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
    ASSERT_NE(StringName{}, StringName{c_LongStringToAvoidSSO});
}

TEST(StringName, DefaultConstructedIsNotEqualToANonBlankStringNameReversedOp)
{
    ASSERT_NE(StringName{c_LongStringToAvoidSSO}, StringName{});
}

TEST(StringName, DefaultConstructedIsNotEqualToANonBlankStringView)
{
    ASSERT_NE(StringName{}, std::string_view{c_LongStringToAvoidSSO});
}

TEST(StringName, DefaultConstructedIsNotEqualToANonBlankStringViewReversedOp)
{
    ASSERT_NE(std::string_view{c_LongStringToAvoidSSO}, StringName{});
}

TEST(StringName, DefaultConstructedIsNotEqualToANonBlankString)
{
    ASSERT_NE(StringName{}, std::string{c_LongStringToAvoidSSO});
}

TEST(StringName, DefaultConstructedIsNotEqualToANonBlankStringReversedOp)
{
    ASSERT_NE(std::string{c_LongStringToAvoidSSO}, StringName{});
}

TEST(StringName, DefaultConstructedIsNotEqualToANonBlankCString)
{
    ASSERT_NE(StringName{}, c_LongStringToAvoidSSO);
}

TEST(StringName, DefaultConstructedIsNotEqualToANonBlankCStringReversedOp)
{
    ASSERT_NE(c_LongStringToAvoidSSO, StringName{});
}

TEST(StringName, DefaultConstructedComparesLessThanContentfulStringView)
{
    ASSERT_LT(StringName{}, StringName{c_LongStringToAvoidSSO});
}

TEST(StringName, DefaultConstructedComparesLessThanContentfulStringViewReversedOp)
{
    ASSERT_LT(StringName{}, StringName{c_LongStringToAvoidSSO});
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
    StringName const aCopy{a};
    StringName b{c_LongStringToAvoidSSO};
    StringName const bCopy{b};

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
    ASSERT_NO_THROW({ StringName(std::string_view(c_LongStringToAvoidSSO)); });
}

TEST(StringName, CanConstructFromString)
{
    ASSERT_NO_THROW({ StringName(std::string{c_LongStringToAvoidSSO}); });
}

TEST(StringName, CanConstructFromCString)
{
    ASSERT_NO_THROW({ StringName("somecstring"); });
}

TEST(StringName, CanImplicitlyConstructFromCString)
{
    auto const f = [](StringName const&) {};
    f("cstring");  // should compile
}

TEST(StringName, CanImplicitlyConstructFromStringView)
{
    auto const f = [](StringName const&) {};
    f(std::string_view{"cstring"});  // should compile
}

TEST(StringName, CanImplicitlyConstructFromCStringView)
{
    auto const f = [](CStringView const&) {};
    f(CStringView{"cstring"});  // should compile
}

TEST(StringName, CopyAssigningOneNonDefaultConstructedStringNameOverAnotherMakesLhsCompareEqual)
{
    StringName a{c_LongStringToAvoidSSO};
    StringName b{c_AnotherStringToAvoidSSO};
    a = b;
    ASSERT_EQ(a, b);
}

TEST(StringName, MoveAssingingOneDefaultConstructedStringNameOverAnotherMakesLhsCompareEqual)
{
    StringName a{c_LongStringToAvoidSSO};
    StringName b{c_AnotherStringToAvoidSSO};
    StringName bTmp{b};
    a = std::move(bTmp);
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
    char const input[] = "string";
    StringName s{input};

    nonstd::span<char const> inputSpan(input);
    nonstd::span<StringName::value_type const> outputSpan{s.data(), std::size(input)};  // plus nul terminator

    ASSERT_TRUE(std::equal(outputSpan.begin(), outputSpan.end(), inputSpan.begin(), inputSpan.end()));
}

TEST(StringName, CStringReturnsNulTerminatedPointerToFirstElement)
{
    char const input[] = "string";
    StringName s{input};

    nonstd::span<char const> inputSpan(input);
    nonstd::span<StringName::value_type const> outputSpan{s.c_str(), std::size(input)};  // plus nul terminator

    ASSERT_TRUE(std::equal(outputSpan.begin(), outputSpan.end(), inputSpan.begin(), inputSpan.end()));
}

TEST(StringName, ImplicitlyConvertingToStringViewWorksAsExpected)
{
    StringName const s{c_LongStringToAvoidSSO};
    ASSERT_EQ(static_cast<std::string_view>(s), std::string_view{c_LongStringToAvoidSSO});
}

TEST(StringName, ImplicitlyConvertingToCStringViewWorksAsExpected)
{
    StringName const s{c_LongStringToAvoidSSO};
    ASSERT_EQ(static_cast<CStringView>(s), CStringView{c_LongStringToAvoidSSO});
}

TEST(StringName, BeginNotEqualToEndForNonEmptyString)
{
    StringName s{c_LongStringToAvoidSSO};
    ASSERT_NE(s.begin(), s.end());
}

TEST(StringName, CBeginNotEqualToCendForNonEmptyString)
{
    StringName s{c_LongStringToAvoidSSO};
    ASSERT_NE(s.cbegin(), s.cend());
}

TEST(StringName, BeginIsEqualToCBeginForNonEmptyString)
{
    StringName s{c_LongStringToAvoidSSO};
    ASSERT_EQ(s.begin(), s.cbegin());
}

TEST(StringName, EndIsEqualToCendForNonEmptyString)
{
    StringName s{c_LongStringToAvoidSSO};
    ASSERT_EQ(s.end(), s.cend());
}

TEST(StringName, EmptyReturnsFalseForNonEmptyString)
{
    StringName s{c_LongStringToAvoidSSO};
    ASSERT_FALSE(s.empty());
}

TEST(StringName, SizeReturnsExpectedValue)
{
    StringName s{c_LongStringToAvoidSSO};
    ASSERT_EQ(s.size(), c_LongStringToAvoidSSOData.size()-1);  // minus nul
}

TEST(StringName, SwapSwapsTheStringNamesContents)
{
    StringName a{c_LongStringToAvoidSSO};
    StringName b{c_AnotherStringToAvoidSSO};
    a.swap(b);
    ASSERT_EQ(a, c_AnotherStringToAvoidSSO);
    ASSERT_EQ(b, c_LongStringToAvoidSSO);
}

TEST(StringName, NonEmptyStringNameComapresEqualToAnotherLogicallyEquivalentNonEmptyStringName)
{
    ASSERT_EQ(StringName{c_LongStringToAvoidSSO}, StringName{c_LongStringToAvoidSSO});
}

TEST(StringName, NonEmptyStringNameComparesEquivalentToStringView)
{
    ASSERT_EQ(StringName{c_LongStringToAvoidSSO}, std::string_view{c_LongStringToAvoidSSO});
}

TEST(StringName, NonEmptyStringNameComparesEquivalentToStringViewReversedOp)
{
    ASSERT_EQ(std::string_view{c_LongStringToAvoidSSO}, StringName{c_LongStringToAvoidSSO});
}

TEST(StringName, NonEmptyStringNameComparesEquivalentToCString)
{
    ASSERT_EQ(StringName{c_LongStringToAvoidSSO}, c_LongStringToAvoidSSO);
}

TEST(StringName, NonEmptyStringNameComparesEquivalentToCStringReversedOp)
{
    ASSERT_EQ(c_LongStringToAvoidSSO, StringName{c_LongStringToAvoidSSO});
}

TEST(StringName, NonEmptyStringNameComaparesEquivalentToCStringView)
{
    ASSERT_EQ(StringName{c_LongStringToAvoidSSO}, CStringView{c_LongStringToAvoidSSO});
}

TEST(StringName, NonEmptyStringNameComaparesEquivalentToCStringViewReversedOp)
{
    ASSERT_EQ(CStringView{c_LongStringToAvoidSSO}, StringName{c_LongStringToAvoidSSO});
}

TEST(StringName, ComparesNotEqualToInequivalentString)
{
    ASSERT_NE(StringName{c_LongStringToAvoidSSO}, StringName{c_AnotherStringToAvoidSSO});
}

TEST(StringName, ComparesNotEqualToInequivalentStringReversedOp)
{
    ASSERT_NE(StringName{c_AnotherStringToAvoidSSO}, StringName{c_LongStringToAvoidSSO});
}

TEST(StringName, StreamsCorrectlyToOutputString)
{
    std::stringstream ss;
    ss << StringName{c_LongStringToAvoidSSO};
    ASSERT_EQ(ss.str(), c_LongStringToAvoidSSO);
}

TEST(StringName, NonEmptyStringNameHasSameHashAsEquivalentStringName)
{
    ASSERT_EQ(std::hash<StringName>{}(StringName{c_LongStringToAvoidSSO}), std::hash<StringName>{}(StringName{c_LongStringToAvoidSSO}));
}

TEST(StringName, NonEmptyStringNameHasSameHashAsEquivalentString)
{
    ASSERT_EQ(std::hash<StringName>{}(StringName{c_LongStringToAvoidSSO}), std::hash<std::string>{}(std::string{c_LongStringToAvoidSSO}));
}

TEST(StringName, NonEmptyStringNameHasSameHashAsEquivalentStringView)
{
    ASSERT_EQ(std::hash<StringName>{}(StringName{c_LongStringToAvoidSSO}), std::hash<std::string_view>{}(std::string_view{c_LongStringToAvoidSSO}));
}

// TODO: test less than (and reverse ops, and ops with non-StringName, etc.)
// TODO: test greater than (and reverse ops, and ops with non-StringName, etc.)
// TODO: test less than or equal to (and reverse ops, and ops with non-StringName, etc.)
// TODO: test greater than or equal to (and reverse ops, and ops with non-StringName, etc.)
