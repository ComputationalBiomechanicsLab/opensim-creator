#include <oscar/DOM/NodePath.h>

#include <gtest/gtest.h>

#include <array>
#include <iterator>
#include <string>
#include <string_view>
#include <vector>

using namespace osc;


namespace
{
    std::vector<std::string> Slurp(NodePath const& np)
    {
        return {np.begin(), np.end()};
    }
}

TEST(NodePath, CanBeDefaultConstructed)
{
    [[maybe_unused]] NodePath const np;
}

TEST(NodePath, WhenDefaultCoonstructedIsEmpty)
{
    ASSERT_TRUE(NodePath().empty());
}

TEST(NodePath, WhenDefaultConstructedBeginEqualsEnd)
{
    NodePath const np;
    ASSERT_EQ(np.begin(), np.end());
}

TEST(NodePath, WhenDefaultConstructedIsNonAbsolute)
{
    ASSERT_FALSE(NodePath().is_absolute());
}

TEST(NodePath, WhenDefaultConstructedComparesEqualToEmptyStringView)
{
    ASSERT_EQ(NodePath(), std::string_view());
}

TEST(NodePath, WhenDefaultConstructedComparesNotEqualToNonEmptyStringView)
{
    ASSERT_NE(NodePath(), std::string_view("hi"));
}

TEST(NodePath, WhenDefaultConstructedHasSameHashAsStringView)
{
    auto const npHash = std::hash<NodePath>{}(NodePath{});
    auto const svHash = std::hash<std::string_view>{}(std::string_view{});
    ASSERT_EQ(npHash, svHash);
}

TEST(NodePath, CanBeConvertedIntoAStringView)
{
    auto sv = static_cast<std::string_view>(NodePath{});
    ASSERT_EQ(sv, std::string_view());
}

TEST(NodePath, WhenConstructedFromSingleElementReturnsNonEmpty)
{
    ASSERT_FALSE(NodePath("el").empty());
}

TEST(NodePath, WhenConstructedFromSingleElementReturnsNotAbsolute)
{
    ASSERT_FALSE(NodePath("el").is_absolute());
}

TEST(NodePath, WhenConstructedFromSingleElementBeginNotEqualToEnd)
{
    NodePath const np{"el"};
    ASSERT_NE(np.begin(), np.end());
}

TEST(NodePath, WhenConstructedFromSingleElementIteratorsHaveDistanceOfOne)
{
    NodePath const np{"el"};
    ASSERT_EQ(std::distance(np.begin(), np.end()), 1);
}

TEST(NodePath, WhenConstructedFromSingleElementSlurpsIntoExpectedResult)
{
    NodePath const np{"el"};
    std::vector<std::string> const expected = {"el"};

    ASSERT_EQ(Slurp(np), expected);
}

TEST(NodePath, WhenConstructedFromSingleElementComparesEqualToEquivStringView)
{
    ASSERT_EQ(NodePath("el"), std::string_view("el"));
}

TEST(NodePath, WhenConstructedFromSingleElementComparesNotEqualToEmptyStringView)
{
    ASSERT_NE(NodePath("el"), std::string_view());
}

TEST(NodePath, WhenConstructedFromSingleElementComparesNotEqualToSomeOtherString)
{
    ASSERT_NE(NodePath("el"), std::string_view("else"));
}

TEST(NodePath, WhenConstructedFromSingleElementHasSameHashAsEquivalentStringView)
{
    auto const npHash = std::hash<NodePath>{}(NodePath{"el"});
    auto const svHash = std::hash<std::string_view>{}(std::string_view{"el"});
    ASSERT_EQ(npHash, svHash);
}

TEST(NodePath, WhenConstructedFromSingleElementWhenLeadingSlashReturnsNonEmpty)
{
    ASSERT_FALSE(NodePath("/el").empty());
}

TEST(NodePath, WhenConstructedFromSingleElementWithLeadingSlashReturnsIsAbsolute)
{
    ASSERT_TRUE(NodePath("/el").is_absolute());
}

TEST(NodePath, WhenConstructedFromSingleElementWithLeadingSlashComparesEqualToEquivalentStringView)
{
    ASSERT_EQ(NodePath("/el"), std::string_view("/el"));
}

TEST(NodePath, WhenConstructedFromSingleElementWithLeadingSlashHasAnIteratorDistanceOfOne)
{
    NodePath const np{"/el"};
    ASSERT_EQ(std::distance(np.begin(), np.end()), 1);
}

TEST(NodePath, WhenConstructedFromSingleElementWithLeadingSlashSlurpsIntoExpectedVector)
{
    NodePath const np{"/el"};
    std::vector<std::string> const expected = {"el"};
    ASSERT_EQ(Slurp(np), expected);
}

TEST(NodePath, WhenConstructedFromSingleElementWithLeadingSlashHashesToEquivalentStringView)
{
    auto const npHash = std::hash<NodePath>{}(NodePath{"/el"});
    auto const svHash = std::hash<std::string_view>{}(std::string_view{"/el"});
    ASSERT_EQ(npHash, svHash);
}

TEST(NodePath, WhenConstructedFromTwoElementsWithNoLeadingSlashIsNotAbsolute)
{
    ASSERT_FALSE(NodePath("a/b").is_absolute());
}

TEST(NodePath, WhenConstructedFromTwoElementsWithNoLeadingSlashHasIteratorDistanceOfTwo)
{
    NodePath const np{"a/b"};
    ASSERT_EQ(std::distance(np.begin(), np.end()), 2);
}

TEST(NodePath, WhenConstructedFromTwoElementsWithNoLeadingSlashSlurpsIntoExpectedVector)
{
    NodePath const np{"a/b"};
    std::vector<std::string> const expected = {"a", "b"};
    auto slurped = Slurp(np);
    ASSERT_EQ(Slurp(np), expected);
}

TEST(NodePath, WhenConstructedFromTwoElementsWithNoLeadingSlashComparesEqualToEquivalentStringView)
{
    ASSERT_EQ(NodePath("a/b"), std::string_view("a/b"));
}

TEST(NodePath, WhenConstructedFromTwoElementsWithNoLeadingSlashHashesToSameAsStringView)
{
    auto const npHash = std::hash<NodePath>{}(NodePath{"a/b"});
    auto const svHash = std::hash<std::string_view>{}(std::string_view{"a/b"});
    ASSERT_EQ(npHash, svHash);
}

TEST(NodePath, WhenConstructedFromTwoElementsWithLeadingSlashIsAbsolute)
{
    ASSERT_TRUE(NodePath("/a/b").is_absolute());
}

TEST(NodePath, WhenConstructedFromTwoElementsWithLeadingSlashHasIteratorDistanceOfTwo)
{
    NodePath const np{"/a/b"};
    ASSERT_EQ(std::distance(np.begin(), np.end()), 2);
}

TEST(NodePath, WhenConstructedFromTwoElementsWithLeadingSlashSlurpsToExpectedVector)
{
    NodePath const np{"/a/b"};
    std::vector<std::string> const expected = {"a", "b"};
    ASSERT_EQ(Slurp(np), expected);
}

TEST(NodePath, HasExpectedNumberOfElementsForTestInputs)
{
    struct TestCase final {
        std::string_view input;
        NodePath::iterator::difference_type expectedOutput;
    };

    auto const tcs = std::to_array<TestCase>(
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
        NodePath np{tc.input};
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

    auto const tcs = std::to_array<TestCase>(
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
        ASSERT_EQ(NodePath(tc.input), tc.expectedOutput) << std::string_view{tc.input};
    }
}

TEST(NodePath, ThrowsIfGivenInvalidInputs)
{
    auto const inputs = std::to_array<std::string_view>(
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
        ASSERT_ANY_THROW({ NodePath p(input); }) << std::string{"input was: "} + std::string{input};
    }
}
