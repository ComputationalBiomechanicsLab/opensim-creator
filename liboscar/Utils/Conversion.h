#pragma once

#include <concepts>
#include <utility>

namespace osc
{
    // `Converter<T, U>` is a standardized API for explicitly converting `T` to `U`.
    //
    // It is designed similarly to `std::hash<Key>`, where the intent is to make it possible
    // uniformly define additional conversions between types that the developer may not have
    // control over (e.g. library types). The `Converter<T, U>` is chosen based on C++'s
    // template specialization semantics:
    //
    // - If a template specialization of `Converter<T, U>` is available, it is used.
    //
    // - Otherwise, a default implementation is provided which, assuming `t` is a universal
    //   reference of type `T&&`:
    //
    //   - Prefers to call a conversion operator `std::forward<T>(t).operator U()`
    //   - Otherwise, calls `U(std::forward<T>(t))`
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

    // internal detail
    namespace detail
    {
        // Satisfied if `T` has an `operator U()` conversion function.
        template<typename T, typename U>
        concept HasUserDefinedImplicitConversionFunctionTo = requires(const T& v)
        {
            { v.operator U() };
        };

        // Satisfied if `T1` and `T2` have the same type, after removing qualifiers.
        template<typename T1, typename T2>
        concept SameUnderlyingTypeAs = std::same_as<std::remove_cvref_t<T1>, std::remove_cvref_t<T2>>;
    }

    // The generic/fallback implementation of a `Converter`.
    template<typename T, typename U>
    requires (std::constructible_from<U, T> or detail::HasUserDefinedImplicitConversionFunctionTo<T, U>)
    struct Converter<T, U> final {

        // general case: constructs `U` from `T` (can be ambiguous)
        template<typename TVal>
        requires detail::SameUnderlyingTypeAs<TVal, T>
        U operator()(TVal&& value) const
        {
            return U{std::forward<TVal>(value)};
        }

        // specialization: use `U`'s user-defined implicit conversion operator to convert `T` to `U`
        template<typename TVal>
        requires detail::SameUnderlyingTypeAs<TVal, T> and detail::HasUserDefinedImplicitConversionFunctionTo<T, U>  // care: the `and` is needed for this to be classified as a specialized disambiguating overload
        U operator()(TVal&& value) const
        {
            return std::forward<TVal>(value).operator U();
        }
    };

    // internal detail
    namespace detail
    {
        // Satisfied if `Converter<T, U>` can be instantiated by the compiler
        template<typename T, typename U>
        concept HasConverter = requires
        {
            { Converter<T, U>{} };
        };
    }

    // a helper method that converts the provided `T` to a (potentially, qualified value/ref) `U` using a `Converter<T, U>`
    template<typename U, typename T>
    requires detail::HasConverter<std::remove_cvref_t<T>, U>
    constexpr U to(T&& value)
    {
        return Converter<std::remove_cvref_t<T>, U>{}(std::forward<T>(value));
    }
}
