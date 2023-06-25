#pragma once

#include <array>
#include <cstddef>
#include <utility>

namespace osc
{
    template<typename T, typename... Initializers>
    constexpr auto MakeArray(Initializers&&... args) -> std::array<T, sizeof...(args)>
    {
        return {T(std::forward<Initializers>(args))...};
    }
}