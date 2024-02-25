#pragma once

#include <cstddef>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

namespace osc
{
    // template adopted from `glm::mat<>`
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
}
