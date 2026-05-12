#include "data_frame.h"

#include <liboscar/utilities/assertions.h>

#include <algorithm>
#include <cstddef>
#include <cmath>
#include <iomanip>
#include <ostream>
#include <sstream>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

using namespace opyn;
namespace rgs = std::ranges;

namespace
{
    std::unordered_map<std::string, size_t> build_index_lookup(const std::vector<Series>& series)
    {
        std::unordered_map<std::string, size_t> rv;
        rv.reserve(series.size());
        for (size_t i = 0; i < series.size(); ++i) {
            rv.insert_or_assign(std::string{series[i].name()}, i);
        }
        return rv;
    }

    std::vector<Series> build_series_from_separate_vectors(
        std::vector<std::string> column_names,
        std::vector<std::vector<double>> column_data)
    {
        OSC_ASSERT_ALWAYS(column_names.size() == column_data.size() && "The number of column names (headers) must match the number of column data vectors");

        std::vector<Series> rv;
        rv.reserve(column_names.size());
        for (size_t i = 0; i < column_names.size(); ++i) {
            rv.emplace_back(std::move(column_names[i]),std::move(column_data[i]));
        }
        return rv;
    }

    std::string format_double_like_pandas(double val)
    {
        constexpr int precision = 6;

        if (std::isnan(val)) {
            return "NaN";
        }
        if (std::isinf(val)) {
            return "inf";
        }
        if (val == 0.0) {
            return "0.0";
        }

        std::stringstream ss;
        const double abs_val = std::abs(val);

        if (1e-4 <= abs_val and abs_val < 1e6) {
            ss << std::fixed << std::setprecision(precision) << val;

            // Trim trailing zeros
            std::string rv = std::move(ss).str();
            rv.erase(rv.find_last_not_of('0') + 1, std::string::npos);
            if (rv.back() == '.') {
                rv += '0';
            }
            return rv;
        } else {
            ss << std::scientific << std::setprecision(precision) << val;
            return std::move(ss).str();
        }
    }
}

opyn::DataFrame::DataFrame(
    std::vector<std::string> column_names,
    std::vector<std::vector<double>> column_data,
    std::unordered_map<std::string, std::string> attrs) :

    series_{build_series_from_separate_vectors(std::move(column_names), std::move(column_data))},
    column_to_index_lookup_{build_index_lookup(series_)},
    attrs_{std::move(attrs)}
{}

std::vector<std::string> opyn::DataFrame::columns() const
{
    std::vector<std::string> rv;
    rv.reserve(series_.size());
    for (const auto& series : series_) {
        rv.emplace_back(series.name());
    }
    return rv;
}

std::tuple<size_t, size_t> opyn::DataFrame::shape() const
{
    const size_t num_columns = series_.size();
    return num_columns > 0 ?
        std::pair{series_.front().size(), num_columns} :
        std::pair{0uz,                    num_columns};
}

std::unordered_map<std::string, std::string> opyn::DataFrame::attrs() const
{
    return attrs_;
}

opyn::DataFrame::const_reference opyn::DataFrame::operator[](std::string_view name) const
{
    return series_.at(column_to_index_lookup_.at(std::string{name}));
}

std::ostream& opyn::operator<<(std::ostream& out, const DataFrame& data_frame)
{
    // Generate table cell formatted content.
    std::vector<std::vector<std::string>> formatted_columns;
    formatted_columns.reserve(data_frame.size());
    for (const auto& series : data_frame) {
        auto& formatted_column = formatted_columns.emplace_back();
        formatted_column.reserve(series.size() + 1);  // +1 for header

        // Append header
        formatted_column.emplace_back(series.name());

        // Append formatted data rows (+ perhaps ellipsis).
        constexpr size_t num_head_data_rows = 5;
        constexpr size_t num_tail_data_rows = 5;
        constexpr size_t max_data_rows = num_head_data_rows + num_tail_data_rows + 1;  // +1 because it's silly to put ellipsis when there's one more element.
        if (series.size() > max_data_rows) {
            static_assert(max_data_rows >= num_tail_data_rows, "check for underflows here");

            // Print head
            for (size_t i = 0; i < num_head_data_rows; ++i) {
                formatted_column.push_back(format_double_like_pandas(series[i]));
            }

            formatted_column.emplace_back("...");  // Ellipsis to show we have truncated stuff

            // Print tail
            for (size_t i = series.size() - num_tail_data_rows; i < series.size(); ++i) {
                formatted_column.push_back(format_double_like_pandas(series[i]));
            }
        }
        else {
            // Print all values with no ellipsis.
            for (const auto& value : series) {
                formatted_column.push_back(format_double_like_pandas(value));
            }
        }
    }

    // Emit a formatted table with aligned columns.
    std::vector<size_t> column_content_widths;
    column_content_widths.reserve(formatted_columns.size());
    for (const auto& formatted_column : formatted_columns) {
        const size_t max_width = rgs::max_element(formatted_column, {}, &std::string::size)->size();
        column_content_widths.push_back(max_width);
    }

    // Emit above-table header
    const auto [num_rows, num_columns] = data_frame.shape();
    out << "shape: (" << num_rows << ", " << num_columns << ")\n";

    // Emit formatted header row
    out << '|';  // Left line
    if (column_content_widths.empty()) {
        // Edge-case: If there's no headers, emit a blank header column.
        out << "   |";
    }
    OSC_ASSERT(formatted_columns.size() == column_content_widths.size());
    for (size_t i = 0; i < column_content_widths.size(); ++i) {
        const auto& series_name = formatted_columns[i].front();

        out << ' ';  // Left padding
        out << series_name;
        if (series_name.empty()) {
            out << ' ';  // Ensure column is at least 3 units across (markdown minimum).
        }
        out << " |";  // Right padding and line
    }
    out << '\n';

    // Emit alignment rows
    out << '|';  // Left line
    if (column_content_widths.empty()) {
        // Edge-case: If there's no columns, emit a minimum alignment column
        out << "---|";
    }
    for (const auto& column_content_width : column_content_widths) {
        const size_t actual_width = rgs::max(3uz, column_content_width + 2);  // +2 to account for content padding
        out << ':';  // Left-align
        for (size_t i = 1; i < actual_width; ++i) {
            out << '-';
        }
        out << '|';  // Right line
    }
    out << '\n';

    // Emit data rows
    if (num_columns == 0) {
        // Edge-case: If there are no columns, emit a minimum empty column.
        out << "|   |\n";
    }
    else if (num_rows == 0) {
        out << '|';  // Left line
        for (const auto& column_width : column_content_widths) {
            out << ' ';  // Left padding
            const size_t remaining_width = rgs::max(1uz, column_width);
            for (size_t i = 0; i < remaining_width; ++i) {
                out << ' ';  // Filler
            }
            out << " |";  // Right padding and line
        }
        out << '\n';
    }
    for (size_t row = 1; row < num_rows + 1; ++row) {  // start from 1 (first data row)
        out << '|';  // Left line
        for (size_t column = 0; column < num_columns; ++column) {
            out << ' ';  // Left padding
            out << formatted_columns[column][row];
            const size_t remaining_width = rgs::max(1uz, column_content_widths[column] - formatted_columns[column][row].size());
            for (size_t i = 0; i < remaining_width; ++i) {
                out << ' ';
            }
            out << " |";  // Right padding and line
        }
        out << '\n';
    }

    return out;
}
