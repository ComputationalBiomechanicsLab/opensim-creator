#pragma once

#include <liboscar/utilities/c_string_view.h>
#include <liboscar/utilities/copy_on_upd_ptr.h>

#include <string>
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
        osc::CStringView name() const;

        /// Returns the shape (rows) of this `Series`.
        std::tuple<size_t> shape() const;

        /// Returns `true` if this `Series` contains no rows.
        [[nodiscard]] bool empty() const;

        /// Returns the number of rows in this `Series`.
        size_type size() const;

        /// Returns a reference to the element at the specified position `pos`.
        const_reference operator[](size_type pos) const;

        /// Returns an iterator to the first value in `*this`.
        const_iterator begin() const;

        /// Returns an iterator past the last value in `*this`.
        const_iterator end() const;

        /// Returns a pointer to the underlying contiguous array that
        /// backs `*this` (care: this API is unstable and might evolve
        /// over time to support chunking, etc.).
        const double* data() const;

        /// Converts this `Series` into a `std::vector` of its values.
        std::vector<double> to_list() const;

        /// Returns a new `Series` with the same `name` as `*this`, but with
        /// each data value replaced by `f(value)`.
        Series map_elements(const std::function<double(double)>& f) const;
    private:
        friend bool operator==(const Series&, const Series&);
        friend Series operator*(double, const Series&);
        friend Series operator*(const Series&, double);

        class Impl;
        osc::CopyOnUpdPtr<Impl> impl_;
    };

    /// Returns `true` all members of `lhs` compare equivalent to `rhs`.
    ///
    /// Note: This operator confirms to IEEE 754 and C++'s regular type invariants,
    /// which means identity checks are non-reflexive for special values. Therefore,
    /// if either `lhs` or `rhs` contains `NaN`, this operator will return `false`.
    bool operator==(const Series& lhs, const Series& rhs);

    /// Returns a new `Series` with the same name as `rhs`, but with each
    /// of its data values multiplied by `lhs`.
    Series operator*(double lhs, const Series& rhs);

    /// Returns a new `Series` with the same name as `lhs`, but with each
    /// of its data values multiplied by `rhs`.
    Series operator*(const Series& lhs, double rhs);
}
