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
    std::ostream& operator<<(std::ostream& o, const Vec<L, T>& v)
    {
        o << "Vec" << L << '(';
        std::string_view delim;
        for (const T& el : v) {
            o << delim << el;
            delim = ", ";
        }
        o << ')';
        return o;
    }

    template<size_t L, typename T>
    std::string to_string(const Vec<L, T>& v)
    {
        std::stringstream ss;
        ss << v;
        return std::move(ss).str();
    }

    // when handled as a tuple-like object, a `Vec` decomposes into its elements

    template<size_t I, size_t L, typename T>
    constexpr const T& get(const Vec<L, T>& v) { return v[I]; }

    template<size_t I, size_t L, typename T>
    constexpr T& get(Vec<L, T>& v) { return v[I]; }

    template<size_t I, size_t L, typename T>
    constexpr T&& get(Vec<L, T>&& v) { return std::move(v[I]); }

    template<size_t I, size_t L, typename T>
    constexpr const T&& get(const Vec<L, T>&& v) { return std::move(v[I]); }
}

template<size_t L, typename T>
struct std::tuple_size<osc::Vec<L, T>> {
    static inline constexpr size_t value = L;
};

template<size_t I, size_t L, typename T>
struct std::tuple_element<I, osc::Vec<L, T>> {
    using type = T;
};

template<size_t L, typename T>
struct std::hash<osc::Vec<L, T>> final {
    size_t operator()(const osc::Vec<L, T>& v) const
    {
        return osc::HashRange(v);
    }
};
