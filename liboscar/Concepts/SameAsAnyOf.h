#pragma once

#include <concepts>

namespace osc
{
    // Satisfied if `T` is the same as any of `U...`
    template<typename T, typename... U>
    concept SameAsAnyOf = (std::same_as<T, U> or ...);
}
