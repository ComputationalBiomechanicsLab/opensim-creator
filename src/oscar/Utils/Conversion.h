#pragma once

#include <concepts>
#include <utility>

namespace osc
{
    // `Converter<T, U>` provides a standardized method for converting `T` to `U`
    //
    // it is designed similarly to `std::hash<Key>`. The intent is that this makes it
    // possible to uniformly define conversions between types that you might not have control
    // over (e.g. library types)
    //
    // Given each `(T, U)` pair, each specialization of `Converter` is enabled or disabled:
    //
    // - if `std::constructible_from<U, const T&>` is `true`, then a default implementation that
    //   copy- and (if possible) move-constructs the `U` from a `T` value is provided
    //
    // - if a `Converter<T, U>` specialization is provided by the program or the user, it
    //   is enabled
    //
    // - otherwise, it isn't enabled
    //
    // each program-/user-provided specialization of `Converter` should have an `operator()`
    // function that is capable of value- or copy-constructing `U` from `T`:
    //
    //     // minimally, provide these
    //     U operator()(T) const;
    //     U operator()(const T&) const;
    //
    // rvalue overloads may also be provided by specializations:
    //
    //     U operator()(T&&) const;
    //
    // which downstream code _may_ use (it's safest to always provide _at least_ the ability to
    // copy from the `T` value)
    template<typename T, typename U>
    struct Converter;

    // default implementation for types where `std::constructible_from<U, const T&>` is already true
    template<typename T, typename U>
    struct Converter final {
        U operator()(const T& v) const requires std::constructible_from<U, const T&> { return U(v); }
        U operator()(T&& v) const requires std::constructible_from<U, T&&> { return U(std::move(v)); }
    };
}
