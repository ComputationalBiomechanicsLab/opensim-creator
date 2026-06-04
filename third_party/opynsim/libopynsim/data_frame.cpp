#include "data_frame.h"

#include <liboscar/utilities/assertions.h>

#include <algorithm>
#include <functional>
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
        if (not column_data.empty()) {
            const size_t num_rows_in_first_column = column_data.front().size();
            OSC_ASSERT_ALWAYS(rgs::all_of(column_data, [num_rows_in_first_column](const auto& column) { return column.size() == num_rows_in_first_column; }));
        }

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
    return {height(), width()};
}

size_t opyn::DataFrame::height() const
{
    return series_.empty() ? 0uz : series_.front().size();
}

size_t opyn::DataFrame::width() const
{
    return series_.size();
}

std::unordered_map<std::string, std::string> opyn::DataFrame::attrs() const
{
    return attrs_;
}

opyn::DataFrame::const_reference opyn::DataFrame::operator[](std::string_view name) const
{
    return series_.at(column_to_index_lookup_.at(std::string{name}));
}

opyn::DataFrame opyn::DataFrame::with_series(Series series) const
{
    OSC_ASSERT_ALWAYS((this->empty() or this->height() == series.size()) && "DataFrame::with_series was called on a nonempty `DataFrame` with a `Series` of the wrong size");

    DataFrame rv{*this};
    const auto& [index_iterator, inserted] = rv.column_to_index_lookup_.try_emplace(
        std::string{series.name()},
        rv.series_.size()
    );
    if (inserted) {
        rv.series_.push_back(std::move(series));
    } else {
        rv.series_[index_iterator->second] = std::move(series);
    }
    return rv;
}

bool opyn::operator==(const DataFrame&, const DataFrame&)= default;

std::ostream& opyn::operator<<(std::ostream& out, const DataFrame& data_frame)
{
    static constexpr size_t num_head_data_rows = 5;
    static constexpr size_t num_tail_data_rows = 5;
    static constexpr size_t max_data_rows = num_head_data_rows + num_tail_data_rows + 1;  // +1 because it's silly to put ellipsis when there's one more element.
    static constexpr size_t num_head_columns = 4;
    static constexpr size_t num_tail_columns = 2;
    static constexpr size_t max_columns = num_head_columns + num_tail_columns + 1;  // +1 because it's silly to put ellipsis when there's one more element.

    // Represents a single string-formatted column in the table.
    struct FormattedColumn final {
        static FormattedColumn ellipsis(size_t num_rows)
        {
            FormattedColumn rv;
            rv.header = "...";
            rv.content_width = rv.header.size();
            const size_t n = rgs::min(num_rows, max_data_rows);
            rv.rows.reserve(n);
            for (size_t i = 0; i < n; ++i) {
                rv.rows.emplace_back("...");
            }
            return rv;
        }

        FormattedColumn() = default;
        explicit FormattedColumn(const Series& series) :
            header{series.name()},
            content_width{header.size()}
        {
            if (series.size() > max_data_rows) {
                static_assert(max_data_rows >= num_tail_data_rows, "check for underflows here");

                // Print head
                for (size_t i = 0; i < num_head_data_rows; ++i) {
                    append_row(format_double_like_pandas(series[i]));
                }

                // Print ellipsis
                append_row("...");

                // Print tail
                for (size_t i = series.size() - num_tail_data_rows; i < series.size(); ++i) {
                    append_row(format_double_like_pandas(series[i]));
                }
            }
            else {
                // Print all values with no ellipsis.
                for (const auto& value : series) {
                    append_row(format_double_like_pandas(value));
                }
            }
        }

        void append_row(std::string content)
        {
            const auto& v = rows.emplace_back(std::move(content));
            content_width = rgs::max(content_width, v.size());
        }

        size_t num_rows() const { return rows.size(); }

        std::string header;
        size_t content_width = 0;
        std::vector<std::string> rows;
    };

    // Generate table columns' data cells.
    std::vector<FormattedColumn> formatted_columns;
    formatted_columns.reserve(data_frame.size());
    if (data_frame.size() > max_columns) {
        // Generate head columns normally
        for (size_t i = 0; i < num_head_columns; ++i) {
            formatted_columns.emplace_back(data_frame[i]);
        }

        // Generate stubbed ellipsis column
        formatted_columns.emplace_back(FormattedColumn::ellipsis(data_frame[0].size()));

        // Generate tail columns normally
        for (size_t i = data_frame.size() - num_tail_columns; i < data_frame.size(); ++i) {
            formatted_columns.emplace_back(data_frame[i]);
        }
    } else {
        for (const auto& series : data_frame) {
            formatted_columns.emplace_back(series);
        }
    }

    // Emit above-table header.
    {
        const auto [num_rows, num_columns] = data_frame.shape();
        out << "shape: (" << num_rows << ", " << num_columns << ")\n";
    }

    // Emit table header row.
    {
        out << '|';  // Left line
        if (formatted_columns.empty()) {
            // Edge-case: If there's no headers, emit a blank header column (markdown requirement).
            out << "   |";
        }
        for (const auto& c : formatted_columns) {
            const std::string_view header = c.header.empty() ? std::string_view{" "} : c.header;
            out << ' ';  // Left padding
            out << header;
            const auto remaining = static_cast<ptrdiff_t>(c.content_width) - static_cast<ptrdiff_t>(header.size());
            for (ptrdiff_t i = 0; i < remaining; ++i) {
                out << ' ';
            }
            out << " |";  // Right padding + line
        }
        out << '\n';
    }

    // Emit table alignment row.
    {
        // Emit alignment rows
        out << '|';  // Left line
        if (formatted_columns.empty()) {
            // Edge-case: If there's no columns, emit a minimum alignment column (markdown requirement).
            out << "---|";
        }
        for (const auto& c : formatted_columns) {
            out << ':';  // Left-align
            const size_t num_dashes = rgs::max(2uz, c.content_width + 1);  // +1 to account for right padding
            for (size_t i = 0; i < num_dashes; ++i) {
                out << '-';
            }
            out << '|';  // Right line
        }
        out << '\n';
    }

    OSC_ASSERT_ALWAYS(rgs::adjacent_find(formatted_columns, rgs::not_equal_to{}, &FormattedColumn::num_rows) == formatted_columns.end());
    const size_t num_displayed_rows = formatted_columns.empty() ? 0zu : formatted_columns.front().num_rows();

    // Emit table data rows.
    if (formatted_columns.empty()) {
        // Edge-case: If there are no columns, emit a minimum empty column (markdown requirement).

        out << "|   |\n";
    }
    else if (num_displayed_rows == 0) {
        // Edge-case: If there are no rows, emit empty columns for each row (markdown requirement).

        out << '|';  // Left line
        for (const auto& c : formatted_columns) {
            out << ' ';  // Left padding
            const size_t width = rgs::max(1uz, c.content_width);
            for (size_t i = 0; i < width; ++i) {
                out << ' ';  // Filler
            }
            out << " |";  // Right padding and line
        }
        out << '\n';
    }
    for (size_t row = 0; row < num_displayed_rows; ++row) {
        out << '|';  // Left line
        for (size_t column = 0; column < formatted_columns.size(); ++column) {
            const auto& c = formatted_columns[column];

            out << ' ';  // Left padding
            out << c.rows[row];
            const auto remaining = static_cast<ptrdiff_t>(c.content_width) - static_cast<ptrdiff_t>(c.rows[row].size());
            for (ptrdiff_t i = 0; i < remaining; ++i) {
                out << ' ';
            }
            out << " |";  // Right padding and line
        }
        out << '\n';
    }

    return out;
}
