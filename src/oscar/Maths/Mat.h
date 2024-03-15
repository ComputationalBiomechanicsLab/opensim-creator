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
    struct Mat;

    template<size_t C, size_t R, typename T>
    std::ostream& operator<<(std::ostream& o, Mat<C, R, T> const& m)
    {
        for (size_t row = 0; row < R; ++row) {
            std::string_view delim;
            for (size_t col = 0; col < C; ++col) {
                o << delim << m[col][row];
                delim = " ";
            }
            o << '\n';
        }
        return o;
    }

    template<size_t C, size_t R, typename T>
    std::string to_string(Mat<C, R, T> const& m)
    {
        std::stringstream ss;
        ss << m;
        return std::move(ss).str();
    }

    // when handled as-if it were a tuple, decomposes into its columns (vectors)

    template<size_t I, size_t C, size_t R, typename T>
    constexpr typename Mat<C, R, T>::value_type const& get(Mat<C, R, T> const& m)
    {
        return m[I];
    }

    template<size_t I, size_t C, size_t R, typename T>
    constexpr typename Mat<C, R, T>::value_type& get(Mat<C, R, T>& m)
    {
        return m[I];
    }
}

template<size_t C, size_t R, typename T>
struct std::tuple_size<osc::Mat<C, R, T>> {
    static inline constexpr size_t value = C;
};

template<size_t I, size_t C, size_t R, typename T>
struct std::tuple_element<I, osc::Mat<C, R, T>> {
    using type = typename osc::Mat<C, R, T>::value_type;
};
