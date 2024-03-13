#pragma once

#include <oscar/Maths/Vec.h>
#include <oscar/Utils/Algorithms.h>

#include <concepts>
#include <cstddef>
#include <functional>

namespace osc
{
    // returns a vector containing `op(xv)` for each `xv` in `x`
    template<size_t N, typename T, std::invocable<T const&> UnaryOperation>
    constexpr auto map(Vec<N, T> const& x, UnaryOperation op) -> Vec<N, decltype(std::invoke(op, x[0]))>
    {
        Vec<N, decltype(std::invoke(op, x[0]))> rv{};
        for (size_t i = 0; i < N; ++i) {
            rv[i] = std::invoke(op, x[i]);
        }
        return rv;
    }

    // returns a vector containing `op(xv, yv)` for each `(xv, yv)` in `x` and `y`
    template<size_t N, typename T, std::invocable<T const&, T const&> BinaryOperation>
    constexpr auto map(Vec<N, T> const& x, Vec<N, T> const& y, BinaryOperation op) -> Vec<N, decltype(std::invoke(op, x[0], y[0]))>
    {
        Vec<N, decltype(std::invoke(op, x[0], y[0]))> rv{};
        for (size_t i = 0; i < N; ++i) {
            rv[i] = std::invoke(op, x[i], y[i]);
        }
        return rv;
    }

    // returns a vector containing `op(xv, yv, zv)` for each `(xv, yv, zv)` in `x`, `y`, and `z`
    template<size_t N, typename T, std::invocable<T const&, T const&, T const&> TernaryOperation>
    constexpr auto map(Vec<N, T> const& x, Vec<N, T> const& y, Vec<N, T> const& z, TernaryOperation op)
        -> Vec<N, decltype(std::invoke(op, x[0], y[0], z[0]))>
    {
        Vec<N, decltype(std::invoke(op, x[0], y[0], z[0]))> rv{};
        for (size_t i = 0; i < N; ++i) {
            rv[i] = std::invoke(op, x[i], y[i], z[i]);
        }
        return rv;
    }

    // tests if all elements in `v` are `true`
    template<size_t N>
    constexpr bool all_of(Vec<N, bool> const& v)
    {
        return all_of(v, std::identity{});
    }

    // tests if any element in `v` is `true`
    template<size_t N>
    constexpr bool any_of(Vec<N, bool> const& v)
    {
        return any_of(v, std::identity{});
    }

    // tests if no elements in `v` are `true`
    template<size_t N>
    constexpr bool none_of(Vec<N, bool> const& v)
    {
        return none_of(v, std::identity{});
    }
}
