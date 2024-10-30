#include <oscar/Utils/StringHelpers.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

using osc::try_parse_hex_chars_as_byte;
using osc::strip_whitespace;
using osc::to_hex_chars;
using osc::is_valid_identifier;
using osc::join;

TEST(strip_whitespace, works_as_expected)
{
    struct TestCase final {
        std::string_view input;
        std::string_view expected_output;
    };

    const auto test_cases = std::to_array<TestCase>({
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

    for (const auto& [input, expected_output] : test_cases) {
        const std::string_view result = strip_whitespace(input);
        ASSERT_EQ(result, expected_output);
    }
}

// setup standard test cases so that googletest can automatically generate
// one test per test case
namespace
{
    std::ostream& operator<<(std::ostream& o, const std::optional<float>& maybe_float)
    {
        if (maybe_float) {
            o << "Some(" << *maybe_float << ')';
        }
        else {
            o << "None";
        }
        return o;
    }

    struct TestCase final {
        std::string_view input;
        std::optional<float> expected_output;
    };

    std::string with_escaped_control_characters(std::string_view sv)
    {
        std::string rv;
        rv.reserve(sv.size());
        for (auto c : sv) {
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

    std::ostream& operator<<(std::ostream& o, const TestCase& test_case)
    {
        // care: test UIs (e.g. Visual Studio test explorer) don't like it
        //       when they have to print test names containing control
        //       characters
        const std::string sanitized_input = with_escaped_control_characters(test_case.input);
        return o << "TestCase(input = " << sanitized_input << ", expected_output = " << test_case.expected_output << ')';
    }

    void PrintTo(const TestCase& test_case, std::ostream* o)
    {
        // this teaches googletest to pretty-print a std::optional<float>
        //
        // see: googletest/include/gtest/gtest-printers.h `PrintTo`
        *o << test_case;
    }

    constexpr auto c_test_cases = std::to_array<TestCase>({
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
    testing::ValuesIn(c_test_cases)
);

TEST_P(FromCharsStripWhitespace, Check)
{
    const TestCase test_case = GetParam();
    std::optional<float> output = osc::from_chars_strip_whitespace(test_case.input);
    ASSERT_EQ(output, test_case.expected_output);
}
TEST(to_hex_chars, returns_expected_results_when_compared_to_alternative_implementation)
{
    // test by comparing with a custom implementation (think of this double-entry accounting ;))
    for (size_t i = 0; i <= static_cast<size_t>(std::numeric_limits<uint8_t>::max()); ++i) {
        const auto v = static_cast<uint8_t>(i);

        const uint8_t msn = (v>>4) & 0xf;
        const uint8_t lsn = v & 0xf;
        const char msc = static_cast<char>(msn <= 9 ? '0' + msn : 'a' + (msn-10));
        const char lsc = static_cast<char>(lsn <= 9 ? '0' + lsn : 'a' + (lsn-10));

        const auto [a, b] = to_hex_chars(v);

        ASSERT_EQ(a, msc);
        ASSERT_EQ(b, lsc);
    }
}

TEST(to_hex_chars, returns_expected_results)
{
    struct TestCase final {
        uint8_t input;
        std::pair<char, char> expected_output;
    };

    const auto test_cases = std::to_array<TestCase>({
        {0x00, {'0', '0'}},
        {0x0f, {'0', 'f'}},
        {0xf0, {'f', '0'}},
        {0xff, {'f', 'f'}},
        {0x1a, {'1', 'a'}},
        {0x6e, {'6', 'e'}},
        {0xd0, {'d', '0'}},
        {0xef, {'e', 'f'}},
    });

    for (const auto& [input, expected_output] : test_cases) {
        ASSERT_EQ(to_hex_chars(input), expected_output);
    }
}

TEST(try_parse_hex_chars_as_byte, returns_expected_results)
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

TEST(is_valid_identifier, returns_true_for_typical_identifiers)
{
    const auto test_cases = std::to_array({
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

    for (const auto& test_case : test_cases) {
        ASSERT_TRUE(is_valid_identifier(test_case)) << test_case;
    }
}

TEST(is_valid_identifier, returns_false_when_given_an_identifier_with_leading_numbers)
{
    const auto test_cases = std::to_array({
        "1f",
        "2g",
        "3a_snake_case_string",
        "4aCamelCaseString",
        "5AnotherCamelCaseString",
        "6trailing_numbers_007",
        "7TrailingNumbers007",
        "8Inner56Numbers",
    });

    for (const auto& test_case : test_cases) {
        ASSERT_FALSE(is_valid_identifier(test_case)) << test_case;
    }
}

TEST(is_valid_identifier, returns_false_if_given_an_empty_string)
{
    ASSERT_FALSE(is_valid_identifier(std::string_view{}));
}

TEST(is_valid_identifier, return_false_when_given_identifiers_with_invalid_ASCII_characters)
{
    const auto assert_char_cannot_be_used_in_identifier = [](char c)
    {
        {
            const std::string leading = c + std::string{"leading"};
            ASSERT_FALSE(is_valid_identifier(leading)) << leading;
        }
        {
            const std::string trailing = std::string{"trailing"} + c;
            ASSERT_FALSE(is_valid_identifier(trailing)) << trailing;
        }
        {
            const std::string inner = std::string{"inner"} + c + std::string{"usage"};
            ASSERT_FALSE(is_valid_identifier(inner)) << inner;
        }
    };

    const auto invalid_ASCII_ranges = std::to_array<std::pair<int, int>>({
        {0, 0x1F},    // control chars
        {0x20, 0x2F}, // SPC ! " # $ % & ' ( ) * +  ' - /
        {0x3A, 0x40}, // : ; < = > ? @
        {0x5B, 0x5E}, // [ \ ] ^
        // skip 0x5F (_)
        {0x60, 0x60}, // `
        {0x7B, 0x7F}, // { | } ~ DEL
    });

    for (auto [min, max] : invalid_ASCII_ranges) {
        for (auto c = min; c <= max; ++c) {
            assert_char_cannot_be_used_in_identifier(static_cast<char>(c));
        }
    }
}

TEST(join, works_with_a_blank_string)
{
    const auto range = std::vector<std::string>{};
    ASSERT_EQ(join(range, ","), "");
}

TEST(join, works_with_one_element)
{
    ASSERT_EQ(join(std::to_array({1}), ", "), "1");
}

TEST(join, works_with_two_elements)
{
    ASSERT_EQ(join(std::to_array({1, 2}), ", "), "1, 2");
}

TEST(join, works_with_three_elements)
{
    ASSERT_EQ(join(std::to_array({5, 4, 3}), ", "), "5, 4, 3");
}
