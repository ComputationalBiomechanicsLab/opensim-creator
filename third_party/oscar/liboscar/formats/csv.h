#pragma once

#include <iosfwd>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace osc { class FileDialogFilter; }

namespace osc
{
    class CSV final {
    public:
        // Returns a vector of column data if a row could be read; otherwise, returns `std::nullopt`.
        static std::optional<std::vector<std::string>> read_row(std::istream&);

        // Returns `true` if a CSV row was read from the input and written to `r_columns`
        static bool read_row_into_vector(std::istream&, std::vector<std::string>& r_columns);

        // Writes `columns` to the output stream as a UTF-8-encoded text row.
        static void write_row(std::ostream&, std::span<const std::string> columns);

        // Returns a `FileDialogFilter` that filters for CSV file extensions that are supported by
        // this CSV implementation.
        static const FileDialogFilter& file_dialog_filter();
    };
}
