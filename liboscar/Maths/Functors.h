#pragma once

#include <liboscar/Maths/Vector.h>
#include <liboscar/Utils/Algorithms.h>

#include <concepts>
#include <cstddef>
#include <functional>
#include <ranges>

namespace osc
{
    // returns a vector containing `op(xv)` for each `xv` in `x`
    template<size_t N, typename T, std::invocable<const T&> UnaryOperation>
    constexpr auto map(const Vector<N, T>& x, UnaryOperation op) -> Vector<N, decltype(std::invoke(op, x[0]))>
    {
        Vector<N, decltype(std::invoke(op, x[0]))> rv{};
        for (size_t i = 0; i < N; ++i) {
            rv[i] = std::invoke(op, x[i]);
        }
        return rv;
    }

    // returns a vector containing `op(xv, yv)` for each `(xv, yv)` in `x` and `y`
    template<size_t N, typename T, std::invocable<const T&, const T&> BinaryOperation>
    constexpr auto map(const Vector<N, T>& x, const Vector<N, T>& y, BinaryOperation op) -> Vector<N, decltype(std::invoke(op, x[0], y[0]))>
    {
        Vector<N, decltype(std::invoke(op, x[0], y[0]))> rv{};
        for (size_t i = 0; i < N; ++i) {
            rv[i] = std::invoke(op, x[i], y[i]);
        }
        return rv;
    }

    // returns a vector containing `op(xv, yv, zv)` for each `(xv, yv, zv)` in `x`, `y`, and `z`
    template<size_t N, typename T, std::invocable<const T&, const T&, const T&> TernaryOperation>
    constexpr auto map(const Vector<N, T>& x, const Vector<N, T>& y, const Vector<N, T>& z, TernaryOperation op)
        -> Vector<N, decltype(std::invoke(op, x[0], y[0], z[0]))>
    {
        Vector<N, decltype(std::invoke(op, x[0], y[0], z[0]))> rv{};
        for (size_t i = 0; i < N; ++i) {
            rv[i] = std::invoke(op, x[i], y[i], z[i]);
        }
        return rv;
    }

    // tests if all elements in `v` are `true`
    template<size_t N>
    constexpr bool all_of(const Vector<N, bool>& v)
    {
        return std::ranges::all_of(v, std::identity{});
    }

    // tests if any element in `v` is `true`
    template<size_t N>
    constexpr bool any_of(const Vector<N, bool>& v)
    {
        return std::ranges::any_of(v, std::identity{});
    }

    // tests if no elements in `v` are `true`
    template<size_t N>
    constexpr bool none_of(const Vector<N, bool>& v)
    {
        return std::ranges::none_of(v, std::identity{});
    }
}
