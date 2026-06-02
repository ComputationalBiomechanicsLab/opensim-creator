#pragma once

#include <liboscar/utilities/copy_on_upd_ptr.h>

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
        Series();

        /// Constructs a `Series` with `name` and associated `values`.
        Series(std::string name, std::vector<double> values);

        /// Returns the name of this `Series`.
        std::string_view name() const;

        /// Returns the shape (rows) of this `Series`.
        std::tuple<size_t> shape() const;

        /// Returns the number of rows in this `Series`.
        size_type size() const;

        /// Returns a reference to the element at the specified position `pos`.
        const_reference operator[](size_type pos) const;

        /// Returns an iterator to the first value in `*this`.
        const_iterator begin() const;

        /// Returns an iterator past the last value in `*this`.
        const_iterator end() const;

        /// Converts this `Series` into a list of its values.
        std::vector<double> to_list() const;

    private:
        class Impl;
        osc::CopyOnUpdPtr<Impl> impl_;
    };
}
