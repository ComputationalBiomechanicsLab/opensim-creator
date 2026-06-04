#include "data_frame.h"

#include <gtest/gtest.h>
#include <liboscar/utilities/string_helpers.h>

#include <limits>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

using namespace opyn;

TEST(DataFrame, default_constructed_is_empty)
{
    const DataFrame df;
    ASSERT_TRUE(df.empty());
    ASSERT_EQ(df.size(), 0);
}

TEST(DataFrame, operator_bracket_by_name_returns_expected_series)
{
    const std::vector<std::string> column_names = {"x", "y"};
    const std::vector<std::vector<double>> column_data = {
        {0.0, 1.0, 2.0},  // Xs
        {2.0, 4.0, 6.0},  // Ys
    };
    const DataFrame data_frame{column_names, column_data};
    const Series& series = data_frame["x"];

    ASSERT_EQ(series.name(), "x");
    ASSERT_EQ(series.shape(), std::make_tuple(column_data.front().size()));
    ASSERT_EQ(series.to_list(), column_data.front());
}

TEST(DataFrame, string_formatting_works_as_expected_when_empty)
{
    const std::string got = osc::stream_to_string(DataFrame{});
    constexpr std::string_view expected = R"(shape: (0, 0)
|   |
|---|
|   |
)";
    ASSERT_EQ(got, expected);
}

TEST(DataFrame, string_formatting_works_with_single_empty_series)
{
    const DataFrame df{{"column1"}, {{}}};
    const std::string got = osc::stream_to_string(df);

    constexpr std::string_view expected = R"(shape: (0, 1)
| column1 |
|:--------|
|         |
)";
    ASSERT_EQ(got, expected);
}

TEST(DataFrame, string_formatting_works_with_two_empty_series)
{
    const DataFrame df{{"column1", "column2"}, {{}, {}}};
    const std::string got = osc::stream_to_string(df);

    constexpr std::string_view expected = R"(shape: (0, 2)
| column1 | column2 |
|:--------|:--------|
|         |         |
)";
    ASSERT_EQ(got, expected);
}

TEST(DataFrame, string_formatting_works_with_blank_series_name)
{
    const DataFrame df{{"column1", "", "column3"}, {{}, {}, {}}};
    const std::string got = osc::stream_to_string(df);

    constexpr std::string_view expected = R"(shape: (0, 3)
| column1 |   | column3 |
|:--------|:--|:--------|
|         |   |         |
)";
    ASSERT_EQ(got, expected);
}

TEST(DataFrame, equality_returns_true_for_empty_DataFrame)
{
    ASSERT_EQ(DataFrame{}, DataFrame{});
}

TEST(DataFrame, equality_returns_true_if_both_dataframes_contain_same_series)
{
    const DataFrame lhs{
        {"column1",         "column2"},
        {{5.0, 15.0, 25.0}, {3.0, 6.0, 9.0}}
    };
    const DataFrame rhs{
        {"column1",         "column2"},
        {{5.0, 15.0, 25.0}, {3.0, 6.0, 9.0}}
    };

    ASSERT_EQ(lhs, rhs);
}

TEST(DataFrame, equality_returns_false_if_series_data_differs)
{
    const DataFrame lhs{
        {"column1",         "column2"},
        {{5.0, 15.0, 25.0}, {3.0,  6.0, 9.0}}
    };
    const DataFrame rhs{
        {"column1",         "column2"},
        {{5.0, 15.0, 25.0}, {3.0, -6.0, 9.0}}  // note: a middle value differs
    };

    ASSERT_NE(lhs, rhs);
}

TEST(DataFrame, equality_returns_false_if_series_name_differs)
{
    const DataFrame lhs{
        {"column1",         "column2"},
        {{5.0, 15.0, 25.0}, {3.0, 6.0, 9.0}}
    };
    const DataFrame rhs{
        {"column1",         "columnB"},  // note: second column name differs
        {{5.0, 15.0, 25.0}, {3.0, 6.0, 9.0}}
    };

    ASSERT_NE(lhs, rhs);
}

TEST(DataFrame, equality_returns_true_if_both_dataframes_contain_same_attrs)
{
    const DataFrame lhs{
        {"column1",         "column2"},
        {{5.0, 15.0, 25.0}, {3.0, 6.0, 9.0}},
        {{"attr1", "some-metadata"}},
    };
    const DataFrame rhs{
        {"column1",         "column2"},
        {{5.0, 15.0, 25.0}, {3.0, 6.0, 9.0}},
        {{"attr1", "some-metadata"}},
    };

    ASSERT_EQ(lhs, rhs);
}

TEST(DataFrame, equality_returns_false_if_attrs_differ)
{
    const DataFrame lhs{
        {"column1",         "column2"},
        {{5.0, 15.0, 25.0}, {3.0, 6.0, 9.0}},
        {{"attr1", "some-metadata"}},
    };
    const DataFrame rhs{
        {"column1",         "column2"},
        {{5.0, 15.0, 25.0}, {3.0, 6.0, 9.0}},
        {{"attr1", "some-different-metadata"}},  // differs
    };

    ASSERT_NE(lhs, rhs);
}

TEST(DataFrame, equality_returns_false_bytewise_identical_but_nans)
{
    const DataFrame lhs{
        {"column1",         "column2"},
        {{5.0, 15.0, 25.0}, {std::numeric_limits<double>::quiet_NaN(), 6.0, 9.0}},
        {{"attr1", "some-metadata"}},
    };
    const DataFrame rhs{
        {"column1",         "column2"},
        {{5.0, 15.0, 25.0}, {std::numeric_limits<double>::quiet_NaN(), 6.0, 9.0}},  // uh oh: identical, but irreflexive!
        {{"attr1", "some-metadata"}},
    };

    ASSERT_NE(lhs, rhs);
}

TEST(DataFrame, with_series_appends_series_when_new_name_given)
{
    const DataFrame df{{"column1"}, {{1.0, 2.0, 3.0}}};
    const DataFrame got = df.with_series(Series{"column2", {2.0, 4.0, 6.0}});
    const DataFrame expected{{"column1", "column2"}, {{1.0, 2.0, 3.0}, {2.0, 4.0, 6.0}}};

    ASSERT_EQ(got, expected);
}

TEST(DataFrame, with_series_appends_any_length_when_DataFrame_is_initially_empty)
{
    const DataFrame df;
    const DataFrame got = df.with_series(Series{"column1", {1.0, 2.0, 3.0}});  // shouldn't throw: source `DataFrame` is empty
    const DataFrame expected{{"column1"}, {{1.0, 2.0, 3.0}}};

    ASSERT_EQ(got, expected);
}

TEST(DataFrame, with_series_works_with_appending_empty_series_to_DataFrame_with_existing_empty_Series)
{
    const DataFrame df{{"column1"}, {{}}};
    const DataFrame got = df.with_series({"column2", {}});
    const DataFrame expected{{"column1", "column2"}, {{}, {}}};

    ASSERT_EQ(got, expected);
}

TEST(DataFrame, with_series_throws_if_passed_series_with_different_size_to_existing_DataFrame)
{
    const DataFrame df{{"column1"}, {{1.0, 2.0}}};
    ASSERT_ANY_THROW({ df.with_series({"column2", {}});              }) << "Should be disallowed: appended column is shorter";
    ASSERT_ANY_THROW({ df.with_series({"column2", {2.0}});           }) << "Should be disallowed: appended column is shorter";
    ASSERT_NO_THROW ({ df.with_series({"column2", {2.0, 4.0}});      }) << "OK: correct length";
    ASSERT_ANY_THROW({ df.with_series({"column2", {2.0, 4.0, 6.0}}); }) << "Should be disallowed: appended column is longer";
}
