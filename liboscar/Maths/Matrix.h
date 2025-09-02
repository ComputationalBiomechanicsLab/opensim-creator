#pragma once

#include <cstddef>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>

namespace osc
{
    // a column-major matrix containing `C` columns and `R` rows of type-`T` values
    template<typename T, size_t C, size_t R>
    struct Matrix;

    template<typename T, size_t C, size_t R>
    std::ostream& operator<<(std::ostream& o, const Matrix<T, C, R>& m)
    {
        for (size_t row = 0; row < R; ++row) {
            std::string_view delimiter;
            for (size_t column = 0; column < C; ++column) {
                o << delimiter << m[column][row];
                delimiter = " ";
            }
            o << '\n';
        }
        return o;
    }

    template<typename T, size_t C, size_t R>
    std::string to_string(const Matrix<T, C, R>& m)
    {
        std::stringstream ss;
        ss << m;
        return std::move(ss).str();
    }

    // when handled as a tuple-like object, a `Matrix` decomposes into its column `Vector`s

    template<size_t I, typename T, size_t C, size_t R>
    constexpr const typename Matrix<T, C, R>::value_type& get(const Matrix<T, C, R>& m) { return m[I]; }

    template<size_t I, typename T, size_t C, size_t R>
    constexpr typename Matrix<T, C, R>::value_type& get(Matrix<T, C, R>& m) { return m[I]; }

    template<size_t I, typename T, size_t C, size_t R>
    constexpr typename Matrix<T, C, R>::value_type&& get(Matrix<T, C, R>&& m) { return std::move(m[I]); }

    template<size_t I, typename T, size_t C, size_t R>
    constexpr const typename Matrix<T, C, R>::value_type&& get(const Matrix<T, C, R>&& m) { return std::move(m[I]); }
}

template<typename T, size_t C, size_t R>
struct std::tuple_size<osc::Matrix<T, C, R>> {
    static constexpr size_t value = C;
};

template<size_t I, typename T, size_t C, size_t R>
struct std::tuple_element<I, osc::Matrix<T, C, R>> {
    using type = typename osc::Matrix<T, C, R>::value_type;
};
