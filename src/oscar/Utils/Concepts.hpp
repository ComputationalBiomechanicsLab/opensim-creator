#pragma once

#include <concepts>
#include <cstddef>
#include <iterator>
#include <type_traits>

namespace osc
{
    template<typename T, typename... U>
    concept IsAnyOf = (std::same_as<T, U> || ...);

    template<class Sequence>
    concept Range = requires(Sequence s) {
        std::begin(s);
        std::end(s);
    };

    template<class T>
    concept RandomAccessRange = std::random_access_iterator<typename T::iterator>;

    template<class Container>
    concept ContiguousRange =
        RandomAccessRange<Container> &&
        requires(Container& c) {
            { std::data(c) } -> std::same_as<typename Container::pointer>;
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
    concept ObjectRepresentationByte = IsAnyOf<T, unsigned char, std::byte> && sizeof(T) == 1;

    template<class T, class DerefType>
    concept DereferencesTo = requires(T ptr) {
        {*ptr} -> std::convertible_to<DerefType>;
    };
}
