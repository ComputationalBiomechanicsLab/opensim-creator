#pragma once

#include <concepts>
#include <utility>

namespace osc
{
    // `Converter<T, U>` is a standardized method for explicitly converting `T` to `U`
    //
    // It is designed similarly to `std::hash<Key>`, where the intent is to make it possible
    // uniformly define additional conversions between types that the developer may not have
    // control over (e.g. library types). The `Converter<T, U>` is chosen based on C++'s
    // template specialization semantics:
    //
    // - if a `Converter<T, U>` specialization is provided by the program or the user, it
    //   is preferred
    //
    // - otherwise, a default implementation is provided, which, assuming `t` is a universal
    //   reference of type `T&&`:
    //
    //   - prefers to call `std::forward<T>(t).operator U()`
    //   - otherwise, calls `U(std::forward<T>(t))`
    //
    // each program- or user-provided specialization of `Converter` should define a `operator()`
    // member function(s) that are capable of returning an instance of `U` given a `T`, e.g.:
    //
    //     typename<>
    //     struct osc::Converter<A, B> final {
    //         U operator()(T) const;
    //         U operator()(const T&) const;
    //         U operator()(T&&) const;
    //     };
    //
    // note: it's recommended that specializations avoid only providing an rvalue conversion
    //       function, because generic downstream code may not be designed to handle this case
    template<typename T, typename U>
    struct Converter;

    namespace detail
    {
        template<typename T, typename U>
        concept HasUserDefinedImplicitConversionFunctionTo = requires(const T& v)
        {
            { v.operator U() };
        };

        template<typename T1, typename T2>
        concept SameBaseTypeAs = std::same_as<std::remove_cvref_t<T1>, std::remove_cvref_t<T2>>;
    }

    template<typename T, typename U>
    struct Converter final {

        // general case: constructs `U` from `T` (can be ambiguous)
        template<typename TVal>
        requires detail::SameBaseTypeAs<TVal, T>
        U operator()(TVal&& value) const
        {
            return U(std::forward<TVal>(value));
        }

        // specialization: use `U`'s user-defined implicit conversion operator to convert `T` to `U`
        template<typename TVal>
        requires detail::SameBaseTypeAs<TVal, T> and detail::HasUserDefinedImplicitConversionFunctionTo<T, U>
        U operator()(TVal&& value) const
        {
            return std::forward<TVal>(value).operator U();
        }
    };

    // a helper method that converts the provided `T` to a `U` using a `Converter<T, U>`
    template<typename U, typename T>
    constexpr U to(T&& value)
    {
        return Converter<std::remove_cvref_t<T>, U>{}(std::forward<T>(value));
    }
}
