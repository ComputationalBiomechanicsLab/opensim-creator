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

TEST(NodePath, can_be_default_constructed)
{
    [[maybe_unused]] const NodePath node_path;
}

TEST(NodePath, empty_returns_true_on_default_constructed_instance)
{
    ASSERT_TRUE(NodePath().empty());
}

TEST(NodePath, begin_equals_end_on_default_constructed_instance)
{
    const NodePath node_path;
    ASSERT_EQ(node_path.begin(), node_path.end());
}

TEST(NodePath, is_absolute_returns_false_on_default_constructed_instance)
{
    ASSERT_FALSE(NodePath().is_absolute());
}

TEST(NodePath, compares_equal_to_empty_string_view_when_default_constructed)
{
    ASSERT_EQ(NodePath(), std::string_view());
}

TEST(NodePath, compares_not_equal_to_nonempty_string_view_when_default_constructed)
{
    ASSERT_NE(NodePath(), std::string_view("hi"));
}

TEST(NodePath, hash_is_same_as_hash_of_equivalent_string_view)
{
    const auto node_path_hash = std::hash<NodePath>{}(NodePath{});
    const auto string_view_hash = std::hash<std::string_view>{}(std::string_view{});
    ASSERT_EQ(node_path_hash, string_view_hash);
}

TEST(NodePath, can_be_converted_into_a_string_view)
{
    const NodePath node_path;
    auto sv = static_cast<std::string_view>(node_path);
    ASSERT_EQ(sv, std::string_view());
}

TEST(NodePath, empty_returns_false_when_constructed_from_nonempty_string)
{
    ASSERT_FALSE(NodePath("el").empty());
}

TEST(NodePath, is_absolute_returns_false_when_constructed_from_relative_string)
{
    ASSERT_FALSE(NodePath("el").is_absolute());
}

TEST(NodePath, begin_not_equal_to_end_when_constructed_from_nonempty_string)
{
    const NodePath node_path{"el"};
    ASSERT_NE(node_path.begin(), node_path.end());
}

TEST(NodePath, distance_equals_one_when_constructed_from_single_element_string)
{
    const NodePath node_path{"el"};
    ASSERT_EQ(std::distance(node_path.begin(), node_path.end()), 1);
}

TEST(NodePath, compares_equal_to_expected_result_when_constructed_from_single_element_and_slurped_into_vector)
{
    const NodePath node_path{"el"};
    const std::vector<std::string> expected = {"el"};

    ASSERT_EQ(slurp(node_path), expected);
}

TEST(NodePath, compares_equal_to_equivalent_string_view_when_constructed_from_single_element)
{
    ASSERT_EQ(NodePath("el"), std::string_view("el"));
}

TEST(NodePath, compares_not_equal_to_empty_string_view_when_constructed_from_single_element)
{
    ASSERT_NE(NodePath("el"), std::string_view());
}

TEST(NodePath, compares_not_equal_to_some_other_string_view_when_constructed_from_single_element)
{
    ASSERT_NE(NodePath("el"), std::string_view("else"));
}

TEST(NodePath, hash_is_equal_to_hash_of_equivalent_string_view_when_constructed_from_single_element)
{
    const auto node_path_hash = std::hash<NodePath>{}(NodePath{"el"});
    const auto string_view_hash = std::hash<std::string_view>{}(std::string_view{"el"});
    ASSERT_EQ(node_path_hash, string_view_hash);
}

TEST(NodePath, empty_returns_false_when_constructed_from_abs_path_to_single_element)
{
    ASSERT_FALSE(NodePath("/el").empty());
}

TEST(NodePath, is_absolute_returns_true_when_constructed_from_abs_path_to_single_element)
{
    ASSERT_TRUE(NodePath("/el").is_absolute());
}

TEST(NodePath, compares_equal_to_equivalent_string_view_when_constructed_from_abs_path_to_single_element)
{
    ASSERT_EQ(NodePath("/el"), std::string_view("/el"));
}

TEST(NodePath, distance_returns_1_when_constructed_from_abs_path_to_single_element)
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
