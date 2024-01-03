#pragma once

#include <cstddef>

namespace osc
{
    // inspired by: https://codereview.stackexchange.com/questions/269320/c17-typelist-manipulation
    //
    // ... which was inspired by the book: "Modern C++ Design" (A. Alexandrescu, 2002)

    // Typelist: definition

    template<typename...>
    struct Typelist;

    template<>
    struct Typelist<> {};

    template<typename Head, typename... Tails>
    struct Typelist<Head, Tails...> {
        using head = Head;
        using tails = Typelist<Tails...>;
    };

    // Typelist: size (the number of types held within)

    template<typename TList>
    struct TypelistSize;

    template<typename... Types>
    struct TypelistSize<Typelist<Types...>> {
        static constexpr size_t value = sizeof...(Types);
    };

    template<typename TList>
    inline constexpr size_t TypelistSizeV = TypelistSize<TList>::value;

    // Typelist: indexed access

    template<typename TList, size_t Index>
    struct TypeAt;

    template<typename Head, typename... Tails>
    struct TypeAt<Typelist<Head, Tails...>, 0> {
        using type = Head;
    };

    template<typename Head, typename... Tails, size_t Index>
    struct TypeAt<Typelist<Head, Tails...>, Index> {
        static_assert(Index < sizeof...(Tails)+1, "index out of range");
        using type = typename TypeAt<Typelist<Tails...>, Index-1>::type;
    };

    template<typename TList, size_t Index>
    using TypeAtT = typename TypeAt<TList, Index>::type;
}
