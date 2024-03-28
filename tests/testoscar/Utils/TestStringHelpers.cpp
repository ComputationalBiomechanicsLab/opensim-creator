#include <oscar/Utils/StringHelpers.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

using osc::try_parse_hex_chars_as_byte;
using osc::TrimLeadingAndTrailingWhitespace;
using osc::to_hex_chars;
using osc::is_valid_identifier;

TEST(Algorithms, TrimLeadingAndTrailingWhitespaceWorksAsExpected)
{
    struct TestCase final {
        std::string_view input;
        std::string_view expectedOutput;
    };

    auto const testCases = std::to_array<TestCase>(
    {
        // trivial case
        {"", ""},

        // spaces are handled
        {" ", ""},
        {"  ", ""},

        // tabs are handled
        {"\t", ""},
        {"\t\t", ""},

        // newlines are handled
        {"\n", ""},
        {"\n\n", ""},

        // carriage returns are handled
        {"\r", ""},
        {"\r\r", ""},

        // (weird, but here for completeness)
        {"\v", ""},  // vertical tab (VT)
        {"\f", ""},  // feed (FF)

        // mixtures of the above
        {"\r\n", ""},
        {"\r\n\t", ""},
        {"\r\n \t \n", ""},

        // the content is left behind
        {"a", "a"},
        {" a", "a"},
        {"a ", "a"},
        {" a ", "a"},
        {"\r\na ", "a"},
    });

    for (TestCase const& tc : testCases)
    {
        std::string_view const rv = TrimLeadingAndTrailingWhitespace(tc.input);
        ASSERT_EQ(rv, tc.expectedOutput);
    }
}

// setup standard test cases so that googletest can automatically generate
// one test per test case
namespace
{
    std::ostream& operator<<(std::ostream& o, std::optional<float> const& v)
    {
        if (v)
        {
            o << "Some(" << *v << ')';
        }
        else
        {
            o << "None";
        }
        return o;
    }

    struct TestCase final {
        std::string_view input;
        std::optional<float> expectedOutput;
    };

    std::string WithoutControlCharacters(std::string_view v)
    {
        std::string rv;
        rv.reserve(v.size());
        for (auto c : v)
        {
            switch (c) {
            case '\n':
                rv += "\\n";
                break;
            case '\r':
                rv += "\\r";
                break;
            default:
                rv += c;
            }
        }
        return rv;
    }

    std::ostream& operator<<(std::ostream& o, TestCase const& tc)
    {
        // care: test UIs (e.g. Visual Studio test explorer) don't like it
        //       when they have to print test names containing control
        //       characters
        std::string const sanitizedInput = WithoutControlCharacters(tc.input);
        return o << "TestCase(input = " << sanitizedInput << ", expectedOutput = " << tc.expectedOutput << ')';
    }

    void PrintTo(TestCase const& tc, std::ostream* o)
    {
        // this teaches googletest to pretty-print a std::optional<float>
        //
        // see: googletest/include/gtest/gtest-printers.h `PrintTo`
        *o << tc;
    }

    auto constexpr c_TestCases = std::to_array<TestCase>(
    {
        // it strips purely-whitespace strings
        {"", std::nullopt},
        {" ", std::nullopt},
        {"   ", std::nullopt},
        {"\n", std::nullopt},
        {"\r\n", std::nullopt},

        // it returns nullopt on invalid input
        {"a", std::nullopt},
        {"1a", std::nullopt},
        {"1.0x", std::nullopt},

        // it parses standard numbers
        {"0", 0.0f},
        {"1", 1.0f},
        {"-1", -1.0f},
        {"1e0", 1.0f},
        {"-1e0", -1.0f},
        {"1e1", 10.0f},
        {"1e-1", 0.1f},

        // it parses standard numbers after ignoring whitespace
        {"  0", 0.0f},
        {" 1 ", 1.0f},
        {"-1  ", -1.0f},
        {"  1e0", 1.0f},
        {"  -1e0 ", -1.0f},
        {"\n1e1\r ", 10.0f},
        {"\n  \t1e-1\t ", 0.1f},

        // it handles leading plus symbols
        //
        // care: std::from_chars won't do this
        {"+0", 0.0f},
        {" +1", 1.0f},
    });

    // see: googletest/docs/advanced.md "How to write value-parameterized tests"
    class FromCharsStripWhitespace : public testing::TestWithParam<TestCase> {
    };
}

INSTANTIATE_TEST_SUITE_P(
    FromCharsStripWhitespaceChecks,
    FromCharsStripWhitespace,
    testing::ValuesIn(c_TestCases)
);

TEST_P(FromCharsStripWhitespace, Check)
{
    TestCase c = GetParam();
    std::optional<float> rv = osc::FromCharsStripWhitespace(c.input);
    ASSERT_EQ(rv, c.expectedOutput);
}
TEST(to_hex_chars, ReturnsExpectedResultsWhenComparedToAlternateImplementation)
{
    // test by comparing with
    for (size_t i = 0; i <= static_cast<size_t>(std::numeric_limits<uint8_t>::max()); ++i)
    {
        auto const v = static_cast<uint8_t>(i);

        uint8_t const msn = (v>>4) & 0xf;
        uint8_t const lsn = v & 0xf;
        char const msc = static_cast<char>(msn <= 9 ? '0' + msn : 'a' + (msn-10));
        char const lsc = static_cast<char>(lsn <= 9 ? '0' + lsn : 'a' + (lsn-10));

        auto [a, b] = to_hex_chars(v);

        ASSERT_EQ(a, msc);
        ASSERT_EQ(b, lsc);
    }
}

TEST(to_hex_chars, ReturnsExpectedResults)
{
    struct TestCase final {
        uint8_t input;
        std::pair<char, char> expectedOutput;
    };

    auto const testCases = std::to_array<TestCase>(
    {
        {0x00, {'0', '0'}},
        {0x0f, {'0', 'f'}},
        {0xf0, {'f', '0'}},
        {0xff, {'f', 'f'}},
        {0x1a, {'1', 'a'}},
        {0x6e, {'6', 'e'}},
        {0xd0, {'d', '0'}},
        {0xef, {'e', 'f'}},
    });

    for (TestCase const& tc : testCases)
    {
        ASSERT_EQ(to_hex_chars(tc.input), tc.expectedOutput);
    }
}

TEST(try_parse_hex_chars_as_byte, ReturnsExpectedResults)
{
    // parseable cases
    ASSERT_EQ(try_parse_hex_chars_as_byte('0', '0'), 0x00);
    ASSERT_EQ(try_parse_hex_chars_as_byte('0', '1'), 0x01);
    ASSERT_EQ(try_parse_hex_chars_as_byte('1', '0'), 0x10);
    ASSERT_EQ(try_parse_hex_chars_as_byte('1', '1'), 0x11);
    ASSERT_EQ(try_parse_hex_chars_as_byte('f', 'a'), 0xfa);
    ASSERT_EQ(try_parse_hex_chars_as_byte('b', 'e'), 0xbe);

    // case insensitivity
    ASSERT_EQ(try_parse_hex_chars_as_byte('B', 'e'), 0xbe);
    ASSERT_EQ(try_parse_hex_chars_as_byte('b', 'E'), 0xbe);
    ASSERT_EQ(try_parse_hex_chars_as_byte('B', 'C'), 0xbc);
    ASSERT_EQ(try_parse_hex_chars_as_byte('F', 'A'), 0xfa);

    // interesting edge-case from std::strol that we shouldn't allow
    ASSERT_EQ(try_parse_hex_chars_as_byte('0', 'x'), std::nullopt);
    ASSERT_EQ(try_parse_hex_chars_as_byte('0', 'X'), std::nullopt);
    ASSERT_EQ(try_parse_hex_chars_as_byte('0', '8'), 0x08);
    ASSERT_EQ(try_parse_hex_chars_as_byte('-', '1'), std::nullopt);

    // invalid input
    ASSERT_EQ(try_parse_hex_chars_as_byte(' ', 'a'), std::nullopt);
    ASSERT_EQ(try_parse_hex_chars_as_byte('x', 'a'), std::nullopt);
    ASSERT_EQ(try_parse_hex_chars_as_byte('a', '?'), std::nullopt);
    ASSERT_EQ(try_parse_hex_chars_as_byte('\\', '5'), std::nullopt);
}

TEST(StringHelpers, IsValidIdentifierReturnsTrueForTypicalIdentifiers)
{
    auto const testCases = std::to_array(
    {
        "f",
        "g",
        "a_snake_case_string",
        "aCamelCaseString",
        "AnotherCamelCaseString",
        "trailing_numbers_007",
        "TrailingNumbers007",
        "Inner56Numbers",
        "_typically_private",
        "__very_private",
        "__orIfYouLikeCPPThenItsMaybeReserved",
    });

    for (auto const& testCase : testCases)
    {
        ASSERT_TRUE(is_valid_identifier(testCase)) << testCase;
    }
}

TEST(StringHelpers, IsValidIdentifierReturnsFalseWhenGivenAnIdentifierWithLeadingNumbers)
{
    auto const testCases = std::to_array(
    {
        "1f",
        "2g",
        "3a_snake_case_string",
        "4aCamelCaseString",
        "5AnotherCamelCaseString",
        "6trailing_numbers_007",
        "7TrailingNumbers007",
        "8Inner56Numbers",
    });

    for (auto const& testCase : testCases)
    {
        ASSERT_FALSE(is_valid_identifier(testCase)) << testCase;
    }
}

TEST(StringHelpers, IsValidIdentifierReturnsFalseIfGivenAnEmptyString)
{
    ASSERT_FALSE(is_valid_identifier(std::string_view{}));
}

TEST(StringHelpers, IsValidIdentifierReturnsFalseWhenGivenIdentifiersWithInvalidASCIICharacters)
{
    auto const assertCharCannotBeUsedInIdentifier = [](char c)
    {
        {
            std::string const leading = c + std::string{"leading"};
            ASSERT_FALSE(is_valid_identifier(leading)) << leading;
        }
        {
            std::string const trailing = std::string{"trailing"} + c;
            ASSERT_FALSE(is_valid_identifier(trailing)) << trailing;
        }
        {
            std::string const inner = std::string{"inner"} + c + std::string{"usage"};
            ASSERT_FALSE(is_valid_identifier(inner)) << inner;
        }
    };

    auto const invalidRanges = std::to_array<std::pair<int, int>>(
    {
        {0, 0x1F},    // control chars
        {0x20, 0x2F}, // SPC ! " # $ % & ' ( ) * +  ' - /
        {0x3A, 0x40}, // : ; < = > ? @
        {0x5B, 0x5E}, // [ \ ] ^
        // skip 0x5F (_)
        {0x60, 0x60}, // `
        {0x7B, 0x7F}, // { | } ~ DEL
    });

    for (auto [min, max] : invalidRanges)
    {
        for (auto c = min; c <= max; ++c)
        {
            assertCharCannotBeUsedInIdentifier(static_cast<char>(c));
        }
    }
}
