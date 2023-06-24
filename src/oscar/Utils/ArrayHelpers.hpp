#pragma once

#include <array>
#include <cstddef>
#include <utility>

namespace osc
{
    template<typename T, size_t N, typename... Initializers>
    constexpr auto MakeSizedArray(Initializers&&... args) -> std::array<T, sizeof...(args)>
    {
        static_assert(sizeof...(args) == N);
        return {T(std::forward<Initializers>(args))...};
    }

    template<typename T, typename... Initializers>
    constexpr auto MakeArray(Initializers&&... args) -> std::array<T, sizeof...(args)>
    {
        return {T(std::forward<Initializers>(args))...};
    }
}