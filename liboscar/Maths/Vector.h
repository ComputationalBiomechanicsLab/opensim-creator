#pragma once

#include <liboscar/Maths/Scalar.h>
#include <liboscar/Utils/HashHelpers.h>

#include <cstddef>
#include <functional>
#include <ostream>
#include <string_view>

namespace osc
{
    // specialized by `Vector2`, `Vector3`, etc.
    template<size_t L, Scalar T>
    struct Vector;

    template<size_t L, Scalar T>
    std::ostream& operator<<(std::ostream& out, const Vector<L, T>& vec)
    {
        out << "Vector" << L << '(';
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
    constexpr const T& get(const Vector<L, T>& vec) { return vec[I]; }

    template<size_t I, size_t L, Scalar T>
    constexpr T& get(Vector<L, T>& vec) { return vec[I]; }

    template<size_t I, size_t L, Scalar T>
    constexpr T&& get(Vector<L, T>&& vec) { return std::move(vec[I]); }

    template<size_t I, size_t L, Scalar T>
    constexpr const T&& get(const Vector<L, T>&& vec) { return std::move(vec[I]); }
}

template<size_t L, osc::Scalar T>
struct std::tuple_size<osc::Vector<L, T>> {
    static inline constexpr size_t value = L;
};

template<size_t I, size_t L, osc::Scalar T>
struct std::tuple_element<I, osc::Vector<L, T>> {
    using type = T;
};

template<size_t L, osc::Scalar T>
struct std::hash<osc::Vector<L, T>> final {
    size_t operator()(const osc::Vector<L, T>& vec) const noexcept(noexcept(osc::hash_range(vec)))
    {
        return osc::hash_range(vec);
    }
};
