#pragma once

#include <libopynsim/series.h>

#include <cstddef>
#include <iosfwd>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

namespace opyn
{
    /// Represents a sequence of `Series` with optional associated metadata.
    class DataFrame final {
    public:
        using value_type = Series;
        using size_type = size_t;
        using const_reference = const value_type&;
        using const_iterator = std::vector<Series>::const_iterator;

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
        std::tuple<size_t, size_t> shape() const;

        /// Returns the number of rows in `*this`.
        size_t height() const;

        /// Returns the number of `Series` (columns) in `*this`. Equivalent to `size()`.
        size_t width() const;

        /// Returns this `DataFrame`'s metadata (e.g. header key-values).
        std::unordered_map<std::string, std::string> attrs() const;

        /// Returns the `Series` in `*this` that has the name `name`.
        ///
        /// Throws a `std::exception` if `name` cannot be found.
        const_reference operator[](std::string_view name) const;

        /// Returns a reference to the nth series at `pos` in `*this` (by-column).
        const_reference operator[](size_type pos) const { return series_[pos]; }

        /// Returns the number of `Series` (columns) in `*this`.
        size_type size() const { return series_.size(); }

        /// Returns `true` if `*this` contains no `Series`, `false` otherwise.
        [[nodiscard]] bool empty() const { return series_.empty(); }

        /// Returns an iterator to the first `Series` of `*this`.
        const_iterator begin() const { return series_.begin(); }

        /// Returns an iterator past the last `Series` of `*this`.
        const_iterator end() const { return series_.end(); }
    private:
        std::vector<Series> series_;
        std::unordered_map<std::string, size_t> column_to_index_lookup_;
        std::unordered_map<std::string, std::string> attrs_;
    };

    /// Writes a pretty-printed representation of `data_frame` to `out`.
    std::ostream& operator<<(std::ostream& out, const DataFrame& data_frame);
}
