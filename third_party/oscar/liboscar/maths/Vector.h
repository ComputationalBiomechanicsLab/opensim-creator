#pragma once

#include <liboscar/maths/Scalar.h>
#include <liboscar/utils/HashHelpers.h>

#include <cstddef>
#include <functional>
#include <ostream>
#include <string_view>

namespace osc
{
    // specialized by `Vector2`, `Vector3`, etc.
    template<Scalar T, size_t N>
    struct Vector;

    template<Scalar T, size_t N>
    std::ostream& operator<<(std::ostream& out, const Vector<T, N>& vec)
    {
        out << "Vector" << N << '(';
        std::string_view delimiter;
        for (const T& el : vec) {
            out << delimiter << el;
            delimiter = ", ";
        }
        out << ')';
        return out;
    }

    // when handled as a tuple-like object, a `Vec` decomposes into its elements

    template<size_t I, Scalar T, size_t N>
    constexpr const T& get(const Vector<T, N>& vec) { return vec[I]; }

    template<size_t I, Scalar T, size_t N>
    constexpr T& get(Vector<T, N>& vec) { return vec[I]; }

    template<size_t I, Scalar T, size_t N>
    constexpr T&& get(Vector<T, N>&& vec) { return std::move(vec[I]); }

    template<size_t I, Scalar T, size_t N>
    constexpr const T&& get(const Vector<T, N>&& vec) { return std::move(vec[I]); }
}

template<osc::Scalar T, size_t N>
struct std::tuple_size<osc::Vector<T, N>> {
    static inline constexpr size_t value = N;
};

template<size_t I, osc::Scalar T, size_t N>
struct std::tuple_element<I, osc::Vector<T, N>> {
    using type = T;
};

template<osc::Scalar T, size_t N>
struct std::hash<osc::Vector<T, N>> final {
    size_t operator()(const osc::Vector<T, N>& vec) const noexcept(noexcept(osc::hash_range(vec)))
    {
        return osc::hash_range(vec);
    }
};
