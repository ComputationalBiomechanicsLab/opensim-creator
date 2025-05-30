#include "CoordinateDirection.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <concepts>
#include <sstream>

using namespace osc;

TEST(CoordinateDirection, is_regular)
{
    static_assert(std::regular<CoordinateDirection>);
}

TEST(CoordinateDirection, default_constructed_is_positive_X)
{
    static_assert(CoordinateDirection{} == CoordinateDirection::x());
}

TEST(CoordinateDirection, x_compares_equivalent_to_constructing_from_x_CoordinateAxis)
{
    static_assert(CoordinateDirection::x() == CoordinateDirection{CoordinateAxis::x()});
}

TEST(CoordinateDirection, x_y_and_z_are_not_equal_to_eachover)
{
    static_assert(CoordinateDirection::x() != CoordinateDirection::y());
    static_assert(CoordinateDirection::x() != CoordinateDirection::z());
    static_assert(CoordinateDirection::y() != CoordinateDirection::z());
}

TEST(CoordinateDirection, positive_compares_not_equal_to_negative_CoordinateDirection)
{
    static_assert(CoordinateDirection::x() != CoordinateDirection::minus_x());
    static_assert(CoordinateDirection::y() != CoordinateDirection::minus_y());
    static_assert(CoordinateDirection::z() != CoordinateDirection::minus_z());
}

TEST(CoordinateDirection, axis_ignores_positive_vs_negative)
{
    static_assert(CoordinateDirection::x().axis() == CoordinateDirection::minus_x().axis());
    static_assert(CoordinateDirection::y().axis() == CoordinateDirection::minus_y().axis());
    static_assert(CoordinateDirection::z().axis() == CoordinateDirection::minus_z().axis());
}

TEST(CoordinateDirection, unary_minus_works_as_expected)
{
    static_assert(-CoordinateDirection::x() == CoordinateDirection::minus_x());
    static_assert(-CoordinateDirection::y() == CoordinateDirection::minus_y());
    static_assert(-CoordinateDirection::z() == CoordinateDirection::minus_z());
    static_assert(-CoordinateDirection::minus_x() == CoordinateDirection::x());
    static_assert(-CoordinateDirection::minus_y() == CoordinateDirection::y());
    static_assert(-CoordinateDirection::minus_z() == CoordinateDirection::z());
}

TEST(CoordinateDirection, direction_returns_expected_results)
{
    static_assert(CoordinateDirection::x().direction() == 1.0f);
    static_assert(CoordinateDirection::minus_x().direction() == -1.0f);
    static_assert(CoordinateDirection::y().direction() == 1.0f);
    static_assert(CoordinateDirection::minus_y().direction() == -1.0f);
    static_assert(CoordinateDirection::z().direction() == 1.0f);
    static_assert(CoordinateDirection::minus_z().direction() == -1.0f);

    // templated (casted) versions
    static_assert(CoordinateDirection::x().direction<int>() == 1);
    static_assert(CoordinateDirection::x().direction<ptrdiff_t>() == 1z);
    static_assert(CoordinateDirection::x().direction<double>() == 1.0);
    static_assert(CoordinateDirection::minus_x().direction<int>() == -1);
    static_assert(CoordinateDirection::minus_x().direction<ptrdiff_t>() == -1z);
    static_assert(CoordinateDirection::minus_x().direction<double>() == -1.0);
}

TEST(CoordinateDirection, have_an_expected_total_ordering)
{
    constexpr auto expected_order = std::to_array({
        CoordinateDirection::minus_x(),
        CoordinateDirection::x(),
        CoordinateDirection::minus_y(),
        CoordinateDirection::y(),
        CoordinateDirection::minus_z(),
        CoordinateDirection::z(),
    });
    ASSERT_TRUE(std::is_sorted(expected_order.begin(), expected_order.end()));
}

TEST(CoordinateDirection, cross_works_as_expected)
{
    // cross products along same axis (undefined: falls back to first arg)
    static_assert(cross(CoordinateDirection::x(), CoordinateDirection::x()) == CoordinateDirection::x());
    static_assert(cross(CoordinateDirection::x(), CoordinateDirection::minus_x()) == CoordinateDirection::x());
    static_assert(cross(CoordinateDirection::minus_x(), CoordinateDirection::x()) == CoordinateDirection::minus_x());
    static_assert(cross(CoordinateDirection::y(), CoordinateDirection::y()) == CoordinateDirection::y());
    static_assert(cross(CoordinateDirection::y(), CoordinateDirection::minus_y()) == CoordinateDirection::y());
    static_assert(cross(CoordinateDirection::minus_y(), CoordinateDirection::y()) == CoordinateDirection::minus_y());
    static_assert(cross(CoordinateDirection::z(), CoordinateDirection::z()) == CoordinateDirection::z());
    static_assert(cross(CoordinateDirection::z(), CoordinateDirection::minus_z()) == CoordinateDirection::z());
    static_assert(cross(CoordinateDirection::minus_z(), CoordinateDirection::z()) == CoordinateDirection::minus_z());

    // +X on lhs
    static_assert(cross(CoordinateDirection::x(), CoordinateDirection::y()) == CoordinateDirection::z());
    static_assert(cross(CoordinateDirection::x(), CoordinateDirection::minus_y()) == CoordinateDirection::minus_z());
    static_assert(cross(CoordinateDirection::x(), CoordinateDirection::z()) == CoordinateDirection::minus_y());
    static_assert(cross(CoordinateDirection::x(), CoordinateDirection::minus_z()) == CoordinateDirection::y());

    // -X on lhs
    static_assert(cross(CoordinateDirection::minus_x(), CoordinateDirection::y()) == CoordinateDirection::minus_z());
    static_assert(cross(CoordinateDirection::minus_x(), CoordinateDirection::minus_y()) == CoordinateDirection::z());
    static_assert(cross(CoordinateDirection::minus_x(), CoordinateDirection::z()) == CoordinateDirection::y());
    static_assert(cross(CoordinateDirection::minus_x(), CoordinateDirection::minus_z()) == CoordinateDirection::minus_y());

    // +Y on lhs
    static_assert(cross(CoordinateDirection::y(), CoordinateDirection::z()) == CoordinateDirection::x());
    static_assert(cross(CoordinateDirection::y(), CoordinateDirection::minus_z()) == CoordinateDirection::minus_x());
    static_assert(cross(CoordinateDirection::y(), CoordinateDirection::x()) == CoordinateDirection::minus_z());
    static_assert(cross(CoordinateDirection::y(), CoordinateDirection::minus_x()) == CoordinateDirection::z());

    // -Y on lhs
    static_assert(cross(CoordinateDirection::minus_y(), CoordinateDirection::z()) == CoordinateDirection::minus_x());
    static_assert(cross(CoordinateDirection::minus_y(), CoordinateDirection::minus_z()) == CoordinateDirection::x());
    static_assert(cross(CoordinateDirection::minus_y(), CoordinateDirection::x()) == CoordinateDirection::z());
    static_assert(cross(CoordinateDirection::minus_y(), CoordinateDirection::minus_x()) == CoordinateDirection::minus_z());

    // +Z on lhs
    static_assert(cross(CoordinateDirection::z(), CoordinateDirection::x()) == CoordinateDirection::y());
    static_assert(cross(CoordinateDirection::z(), CoordinateDirection::minus_x()) == CoordinateDirection::minus_y());
    static_assert(cross(CoordinateDirection::z(), CoordinateDirection::y()) == CoordinateDirection::minus_x());
    static_assert(cross(CoordinateDirection::z(), CoordinateDirection::minus_y()) == CoordinateDirection::x());

    // -Z on lhs
    static_assert(cross(CoordinateDirection::minus_z(), CoordinateDirection::x()) == CoordinateDirection::minus_y());
    static_assert(cross(CoordinateDirection::minus_z(), CoordinateDirection::minus_x()) == CoordinateDirection::y());
    static_assert(cross(CoordinateDirection::minus_z(), CoordinateDirection::y()) == CoordinateDirection::x());
    static_assert(cross(CoordinateDirection::minus_z(), CoordinateDirection::minus_y()) == CoordinateDirection::minus_x());
}

TEST(CoordinateDirection, try_parse_blank_input_returns_nullopt)
{
    ASSERT_EQ(CoordinateDirection::try_parse(""), std::nullopt);
}

TEST(CoordinateDirection, try_parse_value_initialized_input_returns_nullopt)
{
    ASSERT_EQ(CoordinateDirection::try_parse({}), std::nullopt);
}

// parsing test cases
namespace
{
    struct ParsingTestCase final {
        std::string_view input;
        std::optional<CoordinateDirection> expected;
    };

    constexpr auto c_parsing_test_cases = std::to_array<ParsingTestCase>({
        // blank/value-initialized
        {"", std::nullopt},
        {{}, std::nullopt},

        // x
        {"x", CoordinateDirection::x()},
        {"X", CoordinateDirection::x()},
        {"+x", CoordinateDirection::x()},
        {"+X", CoordinateDirection::x()},
        {"-x", CoordinateDirection::minus_x()},
        {"-X", CoordinateDirection::minus_x()},

        // y
        {"y", CoordinateDirection::y()},
        {"Y", CoordinateDirection::y()},
        {"+y", CoordinateDirection::y()},
        {"+Y", CoordinateDirection::y()},
        {"-y", CoordinateDirection::minus_y()},
        {"-Y", CoordinateDirection::minus_y()},

        // z
        {"z", CoordinateDirection::z()},
        {"Z", CoordinateDirection::z()},
        {"+z", CoordinateDirection::z()},
        {"+Z", CoordinateDirection::z()},
        {"-z", CoordinateDirection::minus_z()},
        {"-Z", CoordinateDirection::minus_z()},

        // just the +/-
        {"+", std::nullopt},
        {"-", std::nullopt},

        // invalid suffix
        {"xenomorph", std::nullopt},
        {"yelp", std::nullopt},
        {"zodiac", std::nullopt},

        // invalid suffix after a minus
        {"-xy", std::nullopt},
        {"-yz", std::nullopt},
        {"-zebra", std::nullopt},

        // padding is invalid (the caller should remove it)
        {" x", std::nullopt},
        {"x ", std::nullopt},
    });

    class ParsingTestFixture : public testing::TestWithParam<ParsingTestCase> {};
}

INSTANTIATE_TEST_SUITE_P(
    CoordinateDirectionParsingTest,
    ParsingTestFixture,
    testing::ValuesIn(c_parsing_test_cases)
);

TEST_P(ParsingTestFixture, Check)
{
    const ParsingTestCase test_case =  GetParam();
    ASSERT_EQ(CoordinateDirection::try_parse(test_case.input), test_case.expected) << "input = " << test_case.input;
}

// printing test cases
namespace
{
    struct PrintingTestCase final {
        CoordinateDirection input;
        std::string_view expected;
    };

    constexpr auto c_printing_test_cases = std::to_array<PrintingTestCase>({
        {CoordinateDirection::x(), "x"},
        {CoordinateDirection::minus_x(), "-x"},
        {CoordinateDirection::y(), "y"},
        {CoordinateDirection::minus_y(), "-y"},
        {CoordinateDirection::z(), "z"},
        {CoordinateDirection::minus_z(), "-z"},
    });

    class PrintingTestFixture : public testing::TestWithParam<PrintingTestCase> {};
}

INSTANTIATE_TEST_SUITE_P(
    CoordinateDirectionPrintingTest,
    PrintingTestFixture,
    testing::ValuesIn(c_printing_test_cases)
);

TEST_P(PrintingTestFixture, Check)
{
    const PrintingTestCase test_case = GetParam();
    std::stringstream ss;
    ss << test_case.input;
    ASSERT_EQ(ss.str(), test_case.expected);
}

TEST(CoordinateDirection, is_negated_WorksAsExpected)
{
    static_assert(not CoordinateDirection::x().is_negated());
    static_assert(CoordinateDirection::minus_x().is_negated());

    static_assert(not CoordinateDirection::y().is_negated());
    static_assert(CoordinateDirection::minus_y().is_negated());

    static_assert(not CoordinateDirection::z().is_negated());
    static_assert(CoordinateDirection::minus_z().is_negated());
}

TEST(CoordinateDirection, vec_returns_expected_results)
{
    ASSERT_EQ(CoordinateDirection::x().vec(), Vec3(1.0f, 0.0f, 0.0f));
    ASSERT_EQ(CoordinateDirection::y().vec(), Vec3(0.0f, 1.0f, 0.0f));
    ASSERT_EQ(CoordinateDirection::z().vec(), Vec3(0.0f, 0.0f, 1.0f));

    ASSERT_EQ(CoordinateDirection::minus_x().vec(), Vec3(-1.0f, 0.0f, 0.0f));
    ASSERT_EQ(CoordinateDirection::minus_y().vec(), Vec3(0.0f, -1.0f, 0.0f));
    ASSERT_EQ(CoordinateDirection::minus_z().vec(), Vec3(0.0f, 0.0f, -1.0f));
}
