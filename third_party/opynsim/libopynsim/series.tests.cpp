#include "series.h"

#include <gtest/gtest.h>

#include <limits>

using namespace opyn;

TEST(Series, default_constructs_with_empty_name_and_data)
{
    Series series;
    ASSERT_TRUE(series.name().empty());
    ASSERT_TRUE(series.empty());
}

TEST(Series, equality_returns_true_for_two_empty_series)
{
    ASSERT_EQ(Series{}, Series{});
}

TEST(Series, equality_returns_true_for_two_identical_series)
{
    const Series lhs{"x", {1.0, 2.0, 3.0}};
    const Series rhs{"x", {1.0, 2.0, 3.0}};

    ASSERT_EQ(lhs, rhs);
}

TEST(Series, equality_returns_false_if_name_differs)
{
    const Series lhs{"a", {1.0, 2.0, 3.0}};
    const Series rhs{"b", {1.0, 2.0, 3.0}};  // note: different name

    ASSERT_NE(lhs, rhs);
}

TEST(Series, equality_returns_false_if_data_differs)
{
    const Series lhs{"x", {1.0, 2.0, 3.0}};
    const Series rhs{"x", {2.0, 4.0, 6.0}};  // note: different values

    ASSERT_NE(lhs, rhs);
}

TEST(Series, equality_returns_false_if_data_size_differs)
{
    const Series lhs{"x", {1.0, 2.0, 3.0}};
    const Series rhs{"x", {1.0, 2.0, 3.0, 4.0}};  // note: extra member

    ASSERT_NE(lhs, rhs);
}

TEST(Series, equality_returns_false_if_idential_but_contain_nans)
{
    // This behavior is to mirror the rest of the C++ standard library,
    // which tends to treat the `operator==` of aggregates/containers
    // as operating on each member of the aggregate.

    const Series lhs{"x", {1.0, 2.0, std::numeric_limits<double>::quiet_NaN()}};
    const Series rhs{"x", {1.0, 2.0, std::numeric_limits<double>::quiet_NaN()}};

    ASSERT_NE(lhs, rhs);
}

TEST(Series, map_elements_applies_function_to_elements)
{
    const Series input{"x", {1.0, 2.0, 3.0}};
    const Series got = input.map_elements([](double v) { return 2.0*v; });
    const Series expected{"x", {2.0, 4.0, 6.0}};

    ASSERT_EQ(got, expected);
}

TEST(Series, left_multiplied_by_double_returns_expected_series)
{
    const Series input = {"x", {1.0, 2.0, 3.0}};
    const Series got = 2.0 * input;
    const Series expected = {"x", {2.0, 4.0, 6.0}};

    ASSERT_EQ(got, expected);
}

TEST(Series, right_multiplied_by_double_returns_expected_series)
{
    const Series input = {"x", {1.0, 2.0, 3.0}};
    const Series got = input * 2.0;
    const Series expected = {"x", {2.0, 4.0, 6.0}};

    ASSERT_EQ(got, expected);
}
