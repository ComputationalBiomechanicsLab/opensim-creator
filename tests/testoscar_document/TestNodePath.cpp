#include <oscar_document/NodePath.hpp>

#include <gtest/gtest.h>
#include <oscar/Utils/Cpp20Shims.hpp>

#include <array>
#include <iostream>
#include <iterator>
#include <string>
#include <string_view>
#include <vector>


namespace
{
    std::vector<std::string> Slurp(osc::doc::NodePath const& np)
    {
        return std::vector<std::string>(np.begin(), np.end());
    }
}

void PrintTo(const osc::doc::NodePath& np, std::ostream* os)
{
    *os << static_cast<std::string_view>(np);
}

TEST(NodePath, CanBeDefaultConstructed)
{
    [[maybe_unused]] osc::doc::NodePath const np;
}

TEST(NodePath, WhenDefaultCoonstructedIsEmpty)
{
    ASSERT_TRUE(osc::doc::NodePath().empty());
}

TEST(NodePath, WhenDefaultConstructedBeginEqualsEnd)
{
    osc::doc::NodePath const np;
    ASSERT_EQ(np.begin(), np.end());
}

TEST(NodePath, WhenDefaultConstructedIsNonAbsolute)
{
    ASSERT_FALSE(osc::doc::NodePath().isAbsolute());
}

TEST(NodePath, WhenDefaultConstructedComparesEqualToEmptyStringView)
{
    ASSERT_EQ(osc::doc::NodePath(), std::string_view());
}

TEST(NodePath, WhenDefaultConstructedComparesNotEqualToNonEmptyStringView)
{
    ASSERT_NE(osc::doc::NodePath(), std::string_view("hi"));
}

TEST(NodePath, WhenDefaultConstructedHasSameHashAsStringView)
{
    auto const npHash = std::hash<osc::doc::NodePath>{}(osc::doc::NodePath{});
    auto const svHash = std::hash<std::string_view>{}(std::string_view{});
    ASSERT_EQ(npHash, svHash);
}

TEST(NodePath, CanBeConvertedIntoAStringView)
{
    auto sv = static_cast<std::string_view>(osc::doc::NodePath{});
    ASSERT_EQ(sv, std::string_view());
}

TEST(NodePath, WhenConstructedFromSingleElementReturnsNonEmpty)
{
    ASSERT_FALSE(osc::doc::NodePath("el").empty());
}

TEST(NodePath, WhenConstructedFromSingleElementReturnsNotAbsolute)
{
    ASSERT_FALSE(osc::doc::NodePath("el").isAbsolute());
}

TEST(NodePath, WhenConstructedFromSingleElementBeginNotEqualToEnd)
{
    osc::doc::NodePath const np{"el"};
    ASSERT_NE(np.begin(), np.end());
}

TEST(NodePath, WhenConstructedFromSingleElementIteratorsHaveDistanceOfOne)
{
    osc::doc::NodePath const np{"el"};
    ASSERT_EQ(std::distance(np.begin(), np.end()), 1);
}

TEST(NodePath, WhenConstructedFromSingleElementSlurpsIntoExpectedResult)
{
    osc::doc::NodePath const np{"el"};
    std::vector<std::string> const expected = {"el"};

    ASSERT_EQ(Slurp(np), expected);
}

TEST(NodePath, WhenConstructedFromSingleElementComparesEqualToEquivStringView)
{
    ASSERT_EQ(osc::doc::NodePath("el"), std::string_view("el"));
}

TEST(NodePath, WhenConstructedFromSingleElementComparesNotEqualToEmptyStringView)
{
    ASSERT_NE(osc::doc::NodePath("el"), std::string_view());
}

TEST(NodePath, WhenConstructedFromSingleElementComparesNotEqualToSomeOtherString)
{
    ASSERT_NE(osc::doc::NodePath("el"), std::string_view("else"));
}

TEST(NodePath, WhenConstructedFromSingleElementHasSameHashAsEquivalentStringView)
{
    auto const npHash = std::hash<osc::doc::NodePath>{}(osc::doc::NodePath{"el"});
    auto const svHash = std::hash<std::string_view>{}(std::string_view{"el"});
    ASSERT_EQ(npHash, svHash);
}

TEST(NodePath, WhenConstructedFromSingleElementWhenLeadingSlashReturnsNonEmpty)
{
    ASSERT_FALSE(osc::doc::NodePath("/el").empty());
}

TEST(NodePath, WhenConstructedFromSingleElementWithLeadingSlashReturnsIsAbsolute)
{
    ASSERT_TRUE(osc::doc::NodePath("/el").isAbsolute());
}

TEST(NodePath, WhenConstructedFromSingleElementWithLeadingSlashComparesEqualToEquivalentStringView)
{
    ASSERT_EQ(osc::doc::NodePath("/el"), std::string_view("/el"));
}

TEST(NodePath, WhenConstructedFromSingleElementWithLeadingSlashHasAnIteratorDistanceOfOne)
{
    osc::doc::NodePath const np{"/el"};
    ASSERT_EQ(std::distance(np.begin(), np.end()), 1);
}

TEST(NodePath, WhenConstructedFromSingleElementWithLeadingSlashSlurpsIntoExpectedVector)
{
    osc::doc::NodePath const np{"/el"};
    std::vector<std::string> const expected = {"el"};
    ASSERT_EQ(Slurp(np), expected);
}

TEST(NodePath, WhenConstructedFromSingleElementWithLeadingSlashHashesToEquivalentStringView)
{
    auto const npHash = std::hash<osc::doc::NodePath>{}(osc::doc::NodePath{"/el"});
    auto const svHash = std::hash<std::string_view>{}(std::string_view{"/el"});
    ASSERT_EQ(npHash, svHash);
}

TEST(NodePath, WhenConstructedFromTwoElementsWithNoLeadingSlashIsNotAbsolute)
{
    ASSERT_FALSE(osc::doc::NodePath("a/b").isAbsolute());
}

TEST(NodePath, WhenConstructedFromTwoElementsWithNoLeadingSlashHasIteratorDistanceOfTwo)
{
    osc::doc::NodePath const np{"a/b"};
    ASSERT_EQ(std::distance(np.begin(), np.end()), 2);
}

TEST(NodePath, WhenConstructedFromTwoElementsWithNoLeadingSlashSlurpsIntoExpectedVector)
{
    osc::doc::NodePath const np{"a/b"};
    std::vector<std::string> const expected = {"a", "b"};
    auto slurped = Slurp(np);
    ASSERT_EQ(Slurp(np), expected);
}

TEST(NodePath, WhenConstructedFromTwoElementsWithNoLeadingSlashComparesEqualToEquivalentStringView)
{
    ASSERT_EQ(osc::doc::NodePath("a/b"), std::string_view("a/b"));
}

TEST(NodePath, WhenConstructedFromTwoElementsWithNoLeadingSlashHashesToSameAsStringView)
{
    auto const npHash = std::hash<osc::doc::NodePath>{}(osc::doc::NodePath{"a/b"});
    auto const svHash = std::hash<std::string_view>{}(std::string_view{"a/b"});
    ASSERT_EQ(npHash, svHash);
}

TEST(NodePath, WhenConstructedFromTwoElementsWithLeadingSlashIsAbsolute)
{
    ASSERT_TRUE(osc::doc::NodePath("/a/b").isAbsolute());
}

TEST(NodePath, WhenConstructedFromTwoElementsWithLeadingSlashHasIteratorDistanceOfTwo)
{
    osc::doc::NodePath const np{"/a/b"};
    ASSERT_EQ(std::distance(np.begin(), np.end()), 2);
}

TEST(NodePath, WhenConstructedFromTwoElementsWithLeadingSlashSlurpsToExpectedVector)
{
    osc::doc::NodePath const np{"/a/b"};
    std::vector<std::string> const expected = {"a", "b"};
    ASSERT_EQ(Slurp(np), expected);
}

TEST(NodePath, HasExpectedNumberOfElementsForTestInputs)
{
    struct TestCase final {
        std::string_view input;
        osc::doc::NodePath::iterator::difference_type expectedOutput;
    };

    auto const tcs = osc::to_array<TestCase>(
    {
        {"", 0},
        {"/", 0},
        {"a", 1},
        {"/a", 1},
        {"/a/", 1},
        {"a/b", 2},
        {"/a/b", 2},
        {"/a/b/", 2},
        {"/a/b/c", 3},
        {"../", 1},
        {"a/..", 0},
        {"/a/..", 0},
    });

    for (TestCase const& tc : tcs)
    {
        osc::doc::NodePath np{tc.input};
        auto const len = std::distance(np.begin(), np.end());
        ASSERT_EQ(len, tc.expectedOutput);
    }
}

TEST(NodePath, NormalizesInputsAsExpected)
{
    struct TestCase final {
        std::string_view input;
        std::string_view expectedOutput;
    };

    auto const tcs = osc::to_array<TestCase>(
    {
        {"",                       "" },
        {"/",                      "/" },
        {"a/b/c",                  "a/b/c" },
        {"a/..",                   "" },
        {"a/../",                  "" },
        {"a/../c",                 "c" },
        {"a/../c/",                "c" },
        {"/a/../c",                "/c" },
        {"/a/b/../../c",           "/c" },
        {"a/b/../../c",            "c" },
        {"/./././c",               "/c" },
        {"./././c",                "c" },
        {"./",                     "" },
        {".",                      "" },
        {"./.",                    "" },
        {"./a/.",                  "a" },
        {"./a/./",                 "a" },
        {"a//b/.///",              "a/b" },
        {"///",                    "/" },
        {".///",                   "" },
        {"a///b",                  "a/b" },
        {"a/b/c/",                 "a/b/c" },
        {"a/b/c//",                "a/b/c" },
        {"../a/b",                 "../a/b" },
        {"../a/b/",                "../a/b" },
        {"./../a/../",             ".." },
        {"/a/b/c/d",               "/a/b/c/d" },
        {"/a/b/e/f/g/h",           "/a/b/e/f/g/h" },
        {"/a/b",                   "/a/b" },
        {"c/d",                    "c/d" },
        {"e/f/g/h",                "e/f/g/h" },
        {"/a/././b/c/..//d/.././", "/a/b" },
        {"../../../../c/d",        "../../../../c/d" },
        {"/a/b/c/d/../..",         "/a/b"},
        {"/a/././b/c/..//d/.././", "/a/b"},
        {"/a/b/c/d/../..",         "/a/b"},
    });

    for (TestCase const& tc : tcs)
    {
        ASSERT_EQ(osc::doc::NodePath(tc.input), tc.expectedOutput) << std::string_view{tc.input};
    }
}

TEST(NodePath, ThrowsIfGivenInvalidInputs)
{
    auto const inputs = osc::to_array<std::string_view>(
    {
        "a/../..",
        "./a/../..",
        "/..",
        "/./..",
        "/a/../..",
        "/./../",
        "/a/./.././..",
        "/../b/c/d",
        "/a/../../c/d",
        "/../b/c/d",
        "/a/../../c/d",

        // contain invalid chars
        "foo\\bar",
        "a/foo\\bar/c",
        "foo*bar",
        "a/foo*bar*/c",
        "foo+bar",
        "a/foo+bar",
        "foo\tbar",
        "a/b/c/foo\tbar/d",
        "foo\nbar",
        "/a/foo\nbar",
        "/a/b\\/c/",
        "/a+b+c/",
        "/abc*/def/g/",
    });

    for (auto const& input : inputs)
    {
        ASSERT_ANY_THROW({ osc::doc::NodePath p(input); }) << std::string{"input was: "} + std::string{input};
    }
}
