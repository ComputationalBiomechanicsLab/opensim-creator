#pragma once

#include <oscar/Utils/HashHelpers.h>

#include <cstddef>
#include <functional>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

namespace osc
{
    // specialized by `Vec2`, `Vec3`, etc.
    template<size_t L, typename T>
    struct Vec;

    template<size_t L, typename T>
    std::ostream& operator<<(std::ostream& o, Vec<L, T> const& v)
    {
        o << "Vec" << L << '(';
        std::string_view delim;
        for (T const& el : v) {
            o << delim << el;
            delim = ", ";
        }
        o << ')';
        return o;
    }

    template<size_t L, typename T>
    std::string to_string(Vec<L, T> const& v)
    {
        std::stringstream ss;
        ss << v;
        return std::move(ss).str();
    }
}

template<size_t L, typename T>
struct std::hash<osc::Vec<L, T>> final {
    size_t operator()(osc::Vec<L, T> const& v) const
    {
        return osc::HashRange(v);
    }
};
