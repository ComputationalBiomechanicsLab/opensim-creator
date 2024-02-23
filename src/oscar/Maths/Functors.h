#pragma once

#include <oscar/Maths/Vec.h>

#include <concepts>
#include <cstddef>
#include <functional>

namespace osc
{
    template<
        size_t N,
        typename T,
        std::invocable<T const&> UnaryOperation
    >
    constexpr auto map(Vec<N, T> const& v, UnaryOperation op) -> Vec<N, decltype(op(v[0]))>
    {
        Vec<N, decltype(op(v[0]))> rv;
        for (size_t i = 0; i < N; ++i) {
            rv[i] = op(v[i]);
        }
        return rv;
    }

    template<
        size_t N,
        typename T,
        std::invocable<T const&, T const&> BinaryOperation
    >
    constexpr auto map(Vec<N, T> const& v1, Vec<N, T> const& v2, BinaryOperation op) -> Vec<N, decltype(op(v1[0], v2[0]))>
    {
        Vec<N, decltype(op(v1[0], v2[0]))> rv;
        for (size_t i = 0; i < N; ++i) {
            rv[i] = op(v1[i], v2[i]);
        }
        return rv;
    }

    template<
        size_t N,
        typename T,
        std::invocable<T const&, T const&, T const&> TernaryOperation
    >
    constexpr auto map(Vec<N, T> const& v1, Vec<N, T> const& v2, Vec<N, T> const& v3, TernaryOperation op) -> Vec<N, decltype(op(v1[0], v2[0], v3[0]))>
    {
        Vec<N, decltype(op(v1[0], v2[0], v3[0]))> rv;
        for (size_t i = 0; i < N; ++i) {
            rv[i] = op(v1[i], v2[i], v3[i]);
        }
        return rv;
    }

    template<
        size_t N,
        typename T,
        std::invocable<T const&> UnaryPredicate
    >
    constexpr bool all(Vec<N, T> const& v, UnaryPredicate p)
    {
        for (size_t i = 0; i < N; ++i) {
            if (!p(v[i])) {
                return false;
            }
        }
        return true;
    }

    template<size_t N, std::convertible_to<bool> T>
    constexpr bool all(Vec<N, T> const& v)
    {
        return all(v, [](T const& v) { return static_cast<bool>(v); });
    }
}
