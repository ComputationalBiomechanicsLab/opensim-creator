#pragma once

#include <cstddef>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace opyn
{
    /// Represents a two-dimensional table of `doubles`.
    class DataFrame final {
    public:
        /// Constructs an empty `DataFrame`.
        DataFrame() = default;

        /// Constructs a `DataFrame` with the given column names and data.
        ///
        /// Throws an exception if `column_names.size() != column_data.size()`.
        explicit DataFrame(
            std::vector<std::string> column_names,
            std::vector<std::vector<double>> column_data,
            std::unordered_map<std::string, std::string> attrs = {}
        );

        /// Returns this `DataFrame`'s column labels.
        std::vector<std::string> columns() const;

        /// Returns the shape (rows, columns) of this `DataFrame`.
        std::pair<size_t, size_t> shape() const;

        /// Returns this `DataFram`'s metadata (e.g. header key-values).
        std::unordered_map<std::string, std::string> attrs() const;
    private:
        std::vector<std::string> column_names_;
        std::vector<std::vector<double>> column_data_;
        std::unordered_map<std::string, size_t> column_to_index_lookup_;
        std::unordered_map<std::string, std::string> attrs_;
    };
}
