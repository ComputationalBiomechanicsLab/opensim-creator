#pragma once

#include <concepts>

namespace osc
{
    // Satisfied if `T` dereferences to something that is convertible to `DerefType`.
    template<typename T, typename DerefType>
    concept DereferencesTo = requires(T ptr) {
        {*ptr} -> std::convertible_to<DerefType>;
    };
}
