#pragma once

#include <concepts>
#include <utility>

namespace osc
{
    // `Converter<T, U>` is a standardized API for explicitly converting `T` to `U`.
    //
    // It is designed similarly to `std::hash<Key>`, where the intent is to make it possible
    // to uniformly define additional conversions between types that the developer may not
    // have control over (e.g. library types). The `Converter<T, U>` is chosen based on
    // C++'s template specialization semantics:
    //
    // - If a template specialization of `Converter<T, U>` is available, it is used.
    //
    // - Otherwise, a default implementation is provided which, assuming `t` is a universal
    //   reference of type `T&&`, calls `static_cast<U>(std::forward<T>(value))`.
    //
    // Each program- or user-provided specialization of `Converter` should define a `operator()`
    // member function(s) that are capable of returning an instance of `U` given a `T`, e.g.:
    //
    //     typename<>
    //     struct osc::Converter<A, B> final {
    //         U operator()(T) const;
    //         U operator()(const T&) const;
    //         U operator()(T&&) const;
    //     };
    template<typename T, typename U>
    struct Converter;

    // Default `Converter` implementation
    template<typename T, typename U>
    requires std::constructible_from<U, T>
    struct Converter<T, U> final {

        template<typename TVal>
        requires std::same_as<std::remove_cvref_t<TVal>, std::remove_cvref_t<T>>
        U operator()(TVal&& value) const
        {
            return static_cast<U>(std::forward<TVal>(value));
        }
    };

    // internal detail
    namespace detail
    {
        // Satisfied if `Converter<T, U>` can be instantiated by the compiler
        template<typename T, typename U>
        concept HasConverter = requires { { Converter<T, U>{} }; };
    }

    // a helper method that converts the provided `T` to a (potentially, qualified value/ref) `U` using a `Converter<T, U>`
    template<typename U, typename T>
    requires detail::HasConverter<std::remove_cvref_t<T>, U>
    constexpr U to(T&& value)
    {
        return Converter<std::remove_cvref_t<T>, U>{}(std::forward<T>(value));
    }
}
