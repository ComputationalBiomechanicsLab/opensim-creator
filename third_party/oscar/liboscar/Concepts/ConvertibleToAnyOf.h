#pragma once

#include <concepts>

namespace osc
{
    // Satisfied if `T` is convertible to any of `U...`.
    template<typename T, typename... U>
    concept ConvertibleToAnyOf = (std::convertible_to<T, U> or ...);
}
