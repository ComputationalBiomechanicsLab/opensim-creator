#pragma once

#include <concepts>

namespace osc
{
    template<typename T, typename DerefType>
    concept DereferencesTo = requires(T ptr) {
        {*ptr} -> std::convertible_to<DerefType>;
    };
}
