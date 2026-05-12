#include "data_frame.h"

#include <gtest/gtest.h>
#include <liboscar/utilities/string_helpers.h>

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
    const Series series = data_frame["x"];

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
