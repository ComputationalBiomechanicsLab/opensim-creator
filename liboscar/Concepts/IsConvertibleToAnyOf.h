#pragma once

#include <concepts>

namespace osc
{
    template<typename T, typename... U>
    concept IsCovertibleToAnyOf = (std::convertible_to<T, U> || ...);
}
