#include <oscar/Utils/StringHelpers.hpp>

#include <gtest/gtest.h>
#include <oscar/Utils/Cpp20Shims.hpp>

#include <algorithm>
#include <cctype>
#include <iostream>
#include <optional>
#include <string_view>
#include <vector>

TEST(Algorithms, TrimLeadingAndTrailingWhitespaceWorksAsExpected)
{
    struct TestCase final {
        std::string_view input;
        std::string_view expectedOutput;
    };

    auto const testCases = osc::to_array<TestCase>(
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
        std::string_view const rv = osc::TrimLeadingAndTrailingWhitespace(tc.input);
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

    auto constexpr c_TestCases = osc::to_array<TestCase>(
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

TEST(EndsWith, WorksWithBlankString)
{
    ASSERT_TRUE(osc::EndsWith("", ""));
}

TEST(EndsWith, TrueInObviousCases)
{
    ASSERT_TRUE(osc::EndsWith("somefile.osim", "osim"));
    ASSERT_TRUE(osc::EndsWith("sto", "sto"));
}

TEST(EndsWith, FalseInObviousCases)
{
    ASSERT_FALSE(osc::EndsWith("somefile.osim", "sto"));
    ASSERT_FALSE(osc::EndsWith("", " "));
}

TEST(EndsWith, FalseWhenSearchStringIsLonger)
{
    ASSERT_FALSE(osc::EndsWith("somefile.osim", "_somefile.osim"));
}

TEST(ToHexChars, ReturnsExpectedResultsWhenComparedToAlternateImplementation)
{
    // test by comparing with
    for (size_t i = 0; i <= static_cast<size_t>(std::numeric_limits<uint8_t>::max()); ++i)
    {
        uint8_t v = static_cast<uint8_t>(i);

        uint8_t const msn = (v>>4) & 0xf;
        uint8_t const lsn = v & 0xf;
        char msc = msn <= 9 ? '0' + msn : 'a' + (msn-10);
        char lsc = lsn <= 9 ? '0' + lsn : 'a' + (lsn-10);

        auto [a, b] = osc::ToHexChars(v);

        ASSERT_EQ(a, msc);
        ASSERT_EQ(b, lsc);
    }
}

TEST(ToHexChars, ReturnsExpectedResults)
{
    struct TestCase final {
        uint8_t input;
        std::pair<char, char> expectedOutput;
    };

    auto const testCases = osc::to_array<TestCase>(
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
        ASSERT_EQ(osc::ToHexChars(tc.input), tc.expectedOutput);
    }
}

TEST(TryParseHexCharsAsByte, ReturnsExpectedResults)
{
    // parseable cases
    ASSERT_EQ(osc::TryParseHexCharsAsByte('0', '0'), 0x00);
    ASSERT_EQ(osc::TryParseHexCharsAsByte('0', '1'), 0x01);
    ASSERT_EQ(osc::TryParseHexCharsAsByte('1', '0'), 0x10);
    ASSERT_EQ(osc::TryParseHexCharsAsByte('1', '1'), 0x11);
    ASSERT_EQ(osc::TryParseHexCharsAsByte('f', 'a'), 0xfa);
    ASSERT_EQ(osc::TryParseHexCharsAsByte('b', 'e'), 0xbe);

    // case insensitivity
    ASSERT_EQ(osc::TryParseHexCharsAsByte('B', 'e'), 0xbe);
    ASSERT_EQ(osc::TryParseHexCharsAsByte('b', 'E'), 0xbe);
    ASSERT_EQ(osc::TryParseHexCharsAsByte('B', 'C'), 0xbc);
    ASSERT_EQ(osc::TryParseHexCharsAsByte('F', 'A'), 0xfa);

    // interesting edge-case from std::strol that we shouldn't allow
    ASSERT_EQ(osc::TryParseHexCharsAsByte('0', 'x'), std::nullopt);
    ASSERT_EQ(osc::TryParseHexCharsAsByte('0', 'X'), std::nullopt);
    ASSERT_EQ(osc::TryParseHexCharsAsByte('0', '8'), 0x08);
    ASSERT_EQ(osc::TryParseHexCharsAsByte('-', '1'), std::nullopt);

    // invalid input
    ASSERT_EQ(osc::TryParseHexCharsAsByte(' ', 'a'), std::nullopt);
    ASSERT_EQ(osc::TryParseHexCharsAsByte('x', 'a'), std::nullopt);
    ASSERT_EQ(osc::TryParseHexCharsAsByte('a', '?'), std::nullopt);
    ASSERT_EQ(osc::TryParseHexCharsAsByte('\\', '5'), std::nullopt);
}
