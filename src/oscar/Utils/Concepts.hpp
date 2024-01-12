#pragma once

#include <concepts>
#include <cstddef>
#include <type_traits>

namespace osc
{
    template<typename T, typename... U>
    concept IsAnyOf = (std::same_as<T, U> || ...);

    // see:
    //
    // - std::bit_cast (similar constraints)
    // - https://en.cppreference.com/w/cpp/language/object#Object_representation_and_value_representation
    //   > "For TriviallyCopyable types, value representation is a part of the object representation"
    template<class T>
    concept BitCastable = std::copyable<T> && std::is_trivially_destructible_v<T>;

    // see: https://en.cppreference.com/w/cpp/language/object#Object_representation_and_value_representation
    template<class T>
    concept ObjectRepresentationByte = IsAnyOf<T, unsigned char, std::byte>;

    template<class T, class DerefType>
    concept DereferencesTo = requires(T ptr) {
        {*ptr} -> std::convertible_to<DerefType>;
    };

    template<class T>
    concept Hashable = requires(T v) {
        { std::hash<T>{}(v) } -> std::convertible_to<size_t>;
    };
}
