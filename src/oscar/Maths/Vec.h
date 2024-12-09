#pragma once

#include <oscar/Maths/Scalar.h>
#include <oscar/Utils/HashHelpers.h>

#include <cstddef>
#include <functional>
#include <ostream>
#include <string_view>

namespace osc
{
    // specialized by `Vec2`, `Vec3`, etc.
    template<size_t L, Scalar T>
    struct Vec;

    template<size_t L, Scalar T>
    std::ostream& operator<<(std::ostream& out, const Vec<L, T>& vec)
    {
        out << "Vec" << L << '(';
        std::string_view delimiter;
        for (const T& el : vec) {
            out << delimiter << el;
            delimiter = ", ";
        }
        out << ')';
        return out;
    }

    // when handled as a tuple-like object, a `Vec` decomposes into its elements

    template<size_t I, size_t L, Scalar T>
    constexpr const T& get(const Vec<L, T>& vec) { return vec[I]; }

    template<size_t I, size_t L, Scalar T>
    constexpr T& get(Vec<L, T>& vec) { return vec[I]; }

    template<size_t I, size_t L, Scalar T>
    constexpr T&& get(Vec<L, T>&& vec) { return std::move(vec[I]); }

    template<size_t I, size_t L, Scalar T>
    constexpr const T&& get(const Vec<L, T>&& vec) { return std::move(vec[I]); }
}

template<size_t L, osc::Scalar T>
struct std::tuple_size<osc::Vec<L, T>> {
    static inline constexpr size_t value = L;
};

template<size_t I, size_t L, osc::Scalar T>
struct std::tuple_element<I, osc::Vec<L, T>> {
    using type = T;
};

template<size_t L, osc::Scalar T>
struct std::hash<osc::Vec<L, T>> final {
    size_t operator()(const osc::Vec<L, T>& vec) const noexcept(noexcept(osc::hash_range(vec)))
    {
        return osc::hash_range(vec);
    }
};
