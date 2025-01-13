#pragma once

#include <concepts>

namespace osc
{
    // metaclass that has a `value` member equal to `true` if its type argument
    // behaves like a scalar (which is checked when resolving overloads of matrices
    // and vectors)
    template<typename>
    struct IsScalar final {
        static constexpr bool value = false;
    };

    template<std::floating_point T>
    struct IsScalar<T> final {
        static constexpr bool value = true;
    };

    template<std::integral T>
    struct IsScalar<T> final {
        static constexpr bool value = true;
    };

    template<typename T>
    inline constexpr bool IsScalarV = IsScalar<T>::value;

    template<typename T>
    concept Scalar = IsScalarV<T>;

    template<typename T>
    concept ScalarOrBoolean = Scalar<T> or std::same_as<bool, T>;
}
