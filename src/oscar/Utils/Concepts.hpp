#pragma once

#include <type_traits>

namespace osc
{
    template<class Derived, class Base>
    concept DerivedFrom =
        std::is_base_of_v<Base, Derived> &&
        std::is_convertible_v<const Derived*, const Base*>;
}
