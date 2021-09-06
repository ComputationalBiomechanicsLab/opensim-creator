#pragma once

#include <type_traits>
#include <limits>
#include <stdexcept>

namespace osc {
    // a runtime-checked "index" value with support for senteniel values (negative values)
    // that indicate "invalid index"
    //
    // the utility of this is for using undersized index types (e.g. `short`, 32-bit ints,
    // etc.). The perf hit from runtime-checking is typically outweighed by the reduction
    // of memory use, resulting in fewer cache misses, etc..
    //
    // the `Derived` template param is to implement the "curously recurring template
    // pattern" (CRTP) - google it
    template<typename T, typename Derived>
    class CheckedIndex {
    public:
        using value_type = T;
        static constexpr value_type invalid_value = -1;
        static_assert(std::is_signed_v<value_type>);
        static_assert(std::is_integral_v<value_type>);

    private:
        value_type v;

    public:
        // curously-recurring template pattern
        [[nodiscard]] static constexpr Derived from_index(size_t i) {
            if (i > std::numeric_limits<value_type>::max()) {
                throw std::runtime_error{"OSMV error: tried to create a Safe_index with a value that is too high for the underlying value type"};
            }
            return Derived{static_cast<T>(i)};
        }

        constexpr CheckedIndex() noexcept : v{invalid_value} {
        }

        explicit constexpr CheckedIndex(value_type v_) noexcept : v{v_} {
        }

        [[nodiscard]] constexpr value_type get() const noexcept {
            return v;
        }

        [[nodiscard]] constexpr bool is_valid() const noexcept {
            return v >= 0;
        }

        [[nodiscard]] constexpr size_t as_index() const {
            if (!is_valid()) {
                throw std::runtime_error{"OSMV error: tried to convert a Safe_index with an invalid value into an index"};
            }
            return static_cast<size_t>(v);
        }

        [[nodiscard]] constexpr bool operator<(CheckedIndex<T, Derived> other) const noexcept {
            return v < other.v;
        }

        [[nodiscard]] constexpr bool operator==(CheckedIndex<T, Derived> other) const noexcept {
            return v == other.v;
        }

        [[nodiscard]] constexpr bool operator!=(CheckedIndex<T, Derived> other) const noexcept {
            return v != other.v;
        }
    };
}
