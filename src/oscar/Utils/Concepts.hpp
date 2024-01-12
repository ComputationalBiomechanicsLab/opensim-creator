#pragma once

#include <cstddef>
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
        std::is_nothrow_destructible_v<T> &&
        std::is_constructible_v<T, Args...>;

    template<class From, class To>
    concept ConvertibleTo =
        std::is_convertible_v<From, To> &&
        requires { static_cast<To>(std::declval<From>()); };

    template<class F, class... Args>
    concept Invocable = std::is_invocable_v<F, Args...>;

    template<class T, class U>
    concept SameAs = std::is_same_v<T, U>;

    template<class I>
    concept RandomAccessIterator =
        DerivedFrom<typename std::iterator_traits<I>::iterator_category, std::random_access_iterator_tag>;

    template<class T>
    concept RandomAccessContainer = RandomAccessIterator<typename T::iterator>;

    template<class Container>
    concept ContiguousContainer =
        RandomAccessIterator<typename Container::iterator> &&
        requires(Container& c)
        {
            { c.data() } -> SameAs<typename Container::pointer>;
        };

    template<class Sequence>
    concept Iterable = requires(Sequence s)
    {
        std::begin(s);
        std::end(s);
    };

    // see:
    //
    // - std::bit_cast (similar constraints)
    // - https://en.cppreference.com/w/cpp/language/object#Object_representation_and_value_representation
    //   > "For TriviallyCopyable types, value representation is a part of the object representation"
    template<class T>
    concept BitCastable =
        std::is_trivially_copyable_v<T> && std::is_trivially_destructible_v<T>;

    // see: https://en.cppreference.com/w/cpp/language/object#Object_representation_and_value_representation
    template<class T>
    concept ObjectRepresentationByte =
        (SameAs<T, unsigned char> || SameAs<T, std::byte>) && sizeof(T) == 1;

    template<class T, class DerefType>
    concept DereferencesTo = requires(T ptr)
    {
        {*ptr} -> ConvertibleTo<DerefType>;
    };

    template<class T>
    concept Hashable = requires(T v) {
        { std::hash<T>{}(v) } -> ConvertibleTo<size_t>;
    };
}
