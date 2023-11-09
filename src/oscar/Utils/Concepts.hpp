#pragma once

#include <functional>
#include <type_traits>

namespace osc
{
    template<class Derived, class Base>
    concept DerivedFrom =
        std::is_base_of_v<Base, Derived> &&
        std::is_convertible_v<const Derived*, const Base*>;

    template<class T, class... Args>
    concept ConstructibleFrom =
        std::is_destructible_v<T> &&
        std::is_constructible_v<T, Args...>;

    template< class F, class... Args >
    concept Invocable = requires(F&& f, Args&&... args)
    {
        std::invoke(std::forward<F>(f), std::forward<Args>(args)...);
    };
}
