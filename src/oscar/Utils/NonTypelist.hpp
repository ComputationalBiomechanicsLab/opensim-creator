#pragma once

#include <cstddef>

namespace osc
{
    // inspired by the same ideas behind `osc::Typelist`, but for
    // non-type template parameters

    // NonTypelist: definition

    template<typename T, T...>
    struct NonTypelist;

    template<typename T>
    struct NonTypelist<T> {
    };

    template<typename T, T Head, T... Tails>
    struct NonTypelist<T, Head, Tails...> {
        static constexpr T head = Head;
        using tails = NonTypelist<T, Tails...>;
    };

    // NonTypelist: size (the number of compile-time-values held within)

    template<typename NTList>
    struct NonTypelistSize;

    template<typename T, T... Els>
    struct NonTypelistSize<NonTypelist<T, Els...>> {
        static constexpr size_t value = sizeof...(Els);
    };

    template<typename NTList>
    inline constexpr size_t NonTypelistSizeV = NonTypelistSize<NTList>::value;

    // NonTypelist: indexed access

    template<typename NTList, size_t Index>
    struct NonTypeAt;

    template<typename T, T Head, T... Tails>
    struct NonTypeAt<NonTypelist<T, Head, Tails...>, 0> {
        static constexpr T value = Head;
    };

    template<typename T, T Head, T... Tails, size_t Index>
    struct NonTypeAt<NonTypelist<T, Head, Tails...>, Index> {
        static_assert(Index < sizeof...(Tails)+1, "index out of range");
        static constexpr T value = NonTypeAt<NonTypelist<T, Tails...>, Index-1>::value;
    };
}
