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
    template<size_t C, size_t R, typename T>
    struct Matrix;

    template<size_t C, size_t R, typename T>
    std::ostream& operator<<(std::ostream& o, const Matrix<C, R, T>& m)
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

    template<size_t C, size_t R, typename T>
    std::string to_string(const Matrix<C, R, T>& m)
    {
        std::stringstream ss;
        ss << m;
        return std::move(ss).str();
    }

    // when handled as a tuple-like object, a `Mat` decomposes into its column `Vec`s

    template<size_t I, size_t C, size_t R, typename T>
    constexpr const typename Matrix<C, R, T>::value_type& get(const Matrix<C, R, T>& m) { return m[I]; }

    template<size_t I, size_t C, size_t R, typename T>
    constexpr typename Matrix<C, R, T>::value_type& get(Matrix<C, R, T>& m) { return m[I]; }

    template<size_t I, size_t C, size_t R, typename T>
    constexpr typename Matrix<C, R, T>::value_type&& get(Matrix<C, R, T>&& m) { return std::move(m[I]); }

    template<size_t I, size_t C, size_t R, typename T>
    constexpr const typename Matrix<C, R, T>::value_type&& get(const Matrix<C, R, T>&& m) { return std::move(m[I]); }
}

template<size_t C, size_t R, typename T>
struct std::tuple_size<osc::Matrix<C, R, T>> {
    static constexpr size_t value = C;
};

template<size_t I, size_t C, size_t R, typename T>
struct std::tuple_element<I, osc::Matrix<C, R, T>> {
    using type = typename osc::Matrix<C, R, T>::value_type;
};
