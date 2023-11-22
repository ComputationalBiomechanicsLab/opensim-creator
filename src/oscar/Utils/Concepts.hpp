#pragma once

#include <functional>
#include <iterator>
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

    template<class T, class U>
    concept SameAs = std::is_same_v<T, U>;

    template<class I>
    concept RandomAccessIterator =
        DerivedFrom<typename std::iterator_traits<I>::iterator_category, std::random_access_iterator_tag>;

    template<class Container>
    concept ContiguousContainer =
        RandomAccessIterator<typename Container::iterator> &&
        requires(Container& c)
        {
            { c.data() } -> SameAs<typename Container::value_type*>;
        };

    template<class T>
    concept BitCastable =
        std::is_trivially_copyable_v<T> && std::is_trivially_destructible_v<T>;
}
