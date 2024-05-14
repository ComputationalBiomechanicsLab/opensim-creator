#pragma once

#include <iosfwd>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace osc
{
    // returns a vector of column data if a row could be read; otherwise, returns `std::nullopt`
    std::optional<std::vector<std::string>> read_csv_row(
        std::istream&
    );

    // returns `true` if a CSV row was read from the input and written to `r_columns`
    bool read_csv_row_into_vector(
        std::istream&,
        std::vector<std::string>& r_columns
    );

    // writes the given columns to the output stream as a UTF-8-encoded text row
    void write_csv_row(
        std::ostream&,
        std::span<const std::string> columns
    );
}
