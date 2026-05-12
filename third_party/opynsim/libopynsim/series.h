#pragma once

#include <string>
#include <string_view>
#include <tuple>
#include <vector>

namespace opyn
{
    /// Represents a single column of a `DataFrame`.
    class Series final {
    public:
        using value_type = double;
        using size_type = size_t;
        using const_reference = const value_type&;
        using const_iterator = const value_type*;

        /// Constructs an unnamed empty `Series`.
        Series() = default;

        /// Constructs a `Series` with `name` and associated `values`.
        Series(std::string name, std::vector<double> values);

        /// Returns the name of this `Series`.
        std::string_view name() const { return name_; }

        /// Returns the shape (rows) of this `Series`.
        std::tuple<size_t> shape() const { return values_.size(); }

        /// Returns the number of rows in this `Series`.
        size_type size() const { return values_.size(); }

        /// Returns a reference to the element at the specified position `pos`.
        const_reference operator[](size_type pos) const { return values_[pos]; }

        /// Returns an iterator to the first value in `*this`.
        const_iterator begin() const { return values_.data(); }

        /// Returns an iterator past the last value in `*this`.
        const_iterator end() const { return values_.data() + values_.size(); }

        /// Converts this `Series` into a list of its values.
        std::vector<double> to_list() const;
    private:
        std::string name_;
        std::vector<double> values_;
    };
}
