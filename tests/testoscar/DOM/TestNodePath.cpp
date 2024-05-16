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
    std::vector<std::string> slurp(const NodePath& node_path)
    {
        return {node_path.begin(), node_path.end()};
    }
}

TEST(NodePath, CanBeDefaultConstructed)
{
    [[maybe_unused]] const NodePath node_path;
}

TEST(NodePath, WhenDefaultConstructedIsEmpty)
{
    ASSERT_TRUE(NodePath().empty());
}

TEST(NodePath, WhenDefaultConstructedBeginEqualsEnd)
{
    const NodePath node_path;
    ASSERT_EQ(node_path.begin(), node_path.end());
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
    const auto node_path_hash = std::hash<NodePath>{}(NodePath{});
    const auto string_view_hash = std::hash<std::string_view>{}(std::string_view{});
    ASSERT_EQ(node_path_hash, string_view_hash);
}

TEST(NodePath, CanBeConvertedIntoAStringView)
{
    const NodePath node_path;
    auto sv = static_cast<std::string_view>(node_path);
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
    const NodePath node_path{"el"};
    ASSERT_NE(node_path.begin(), node_path.end());
}

TEST(NodePath, WhenConstructedFromSingleElementIteratorsHaveDistanceOfOne)
{
    const NodePath node_path{"el"};
    ASSERT_EQ(std::distance(node_path.begin(), node_path.end()), 1);
}

TEST(NodePath, WhenConstructedFromSingleElementSlurpsIntoExpectedResult)
{
    const NodePath node_path{"el"};
    const std::vector<std::string> expected = {"el"};

    ASSERT_EQ(slurp(node_path), expected);
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
    const auto node_path_hash = std::hash<NodePath>{}(NodePath{"el"});
    const auto string_view_hash = std::hash<std::string_view>{}(std::string_view{"el"});
    ASSERT_EQ(node_path_hash, string_view_hash);
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
    const NodePath node_path{"/el"};
    ASSERT_EQ(std::distance(node_path.begin(), node_path.end()), 1);
}

TEST(NodePath, WhenConstructedFromSingleElementWithLeadingSlashSlurpsIntoExpectedVector)
{
    const NodePath node_path{"/el"};
    const std::vector<std::string> expected = {"el"};
    ASSERT_EQ(slurp(node_path), expected);
}

TEST(NodePath, WhenConstructedFromSingleElementWithLeadingSlashHashesToEquivalentStringView)
{
    const auto node_path_hash = std::hash<NodePath>{}(NodePath{"/el"});
    const auto string_view_hash = std::hash<std::string_view>{}(std::string_view{"/el"});
    ASSERT_EQ(node_path_hash, string_view_hash);
}

TEST(NodePath, WhenConstructedFromTwoElementsWithNoLeadingSlashIsNotAbsolute)
{
    ASSERT_FALSE(NodePath("a/b").is_absolute());
}

TEST(NodePath, WhenConstructedFromTwoElementsWithNoLeadingSlashHasIteratorDistanceOfTwo)
{
    const NodePath node_path{"a/b"};
    ASSERT_EQ(std::distance(node_path.begin(), node_path.end()), 2);
}

TEST(NodePath, WhenConstructedFromTwoElementsWithNoLeadingSlashSlurpsIntoExpectedVector)
{
    const NodePath node_path{"a/b"};
    const std::vector<std::string> expected = {"a", "b"};
    ASSERT_EQ(slurp(node_path), expected);
}

TEST(NodePath, WhenConstructedFromTwoElementsWithNoLeadingSlashComparesEqualToEquivalentStringView)
{
    ASSERT_EQ(NodePath("a/b"), std::string_view("a/b"));
}

TEST(NodePath, WhenConstructedFromTwoElementsWithNoLeadingSlashHashesToSameAsStringView)
{
    const auto node_path_hash = std::hash<NodePath>{}(NodePath{"a/b"});
    const auto string_view_hash = std::hash<std::string_view>{}(std::string_view{"a/b"});
    ASSERT_EQ(node_path_hash, string_view_hash);
}

TEST(NodePath, WhenConstructedFromTwoElementsWithLeadingSlashIsAbsolute)
{
    ASSERT_TRUE(NodePath("/a/b").is_absolute());
}

TEST(NodePath, WhenConstructedFromTwoElementsWithLeadingSlashHasIteratorDistanceOfTwo)
{
    const NodePath node_path{"/a/b"};
    ASSERT_EQ(std::distance(node_path.begin(), node_path.end()), 2);
}

TEST(NodePath, WhenConstructedFromTwoElementsWithLeadingSlashSlurpsToExpectedVector)
{
    const NodePath node_path{"/a/b"};
    const std::vector<std::string> expected = {"a", "b"};
    ASSERT_EQ(slurp(node_path), expected);
}

TEST(NodePath, HasExpectedNumberOfElementsForTestInputs)
{
    struct TestCase final {
        std::string_view input;
        NodePath::iterator::difference_type expectedOutput;
    };

    const auto test_cases = std::to_array<TestCase>({
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

    for (const TestCase& test_case : test_cases) {
        const NodePath node_path{test_case.input};
        const auto len = std::distance(node_path.begin(), node_path.end());
        ASSERT_EQ(len, test_case.expectedOutput);
    }
}

TEST(NodePath, NormalizesInputsAsExpected)
{
    struct TestCase final {
        std::string_view input;
        std::string_view expectedOutput;
    };

    const auto test_cases = std::to_array<TestCase>({
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

    for (const TestCase& test_case : test_cases) {
        ASSERT_EQ(NodePath(test_case.input), test_case.expectedOutput) << std::string_view{test_case.input};
    }
}

TEST(NodePath, ThrowsIfGivenInvalidInputs)
{
    const auto inputs = std::to_array<std::string_view>({
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

    for (const auto& input : inputs) {
        ASSERT_ANY_THROW({ NodePath p(input); }) << std::string{"input was: "} + std::string{input};
    }
}
