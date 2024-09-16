#pragma once

#include <concepts>
#include <cstddef>
#include <functional>
#include <iosfwd>
#include <memory>
#include <ranges>
#include <string_view>
#include <type_traits>

namespace osc
{
    template<typename T, typename... U>
    concept IsAnyOf = (std::same_as<T, U> || ...);

    template<typename T, typename... U>
    concept IsCovertibleToAnyOf = (std::convertible_to<T, U> || ...);

    // see:
    //
    // - std::bit_cast (similar constraints)
    // - https://en.cppreference.com/w/cpp/language/object#Object_representation_and_value_representation
    //   > "For TriviallyCopyable types, value representation is a part of the object representation"
    template<typename T>
    concept BitCastable = std::is_trivially_copyable_v<T> && std::is_trivially_destructible_v<T>;

    // see: https://en.cppreference.com/w/cpp/language/object#Object_representation_and_value_representation
    template<typename T>
    concept ObjectRepresentationByte = IsAnyOf<T, unsigned char, std::byte>;

    template<typename T, typename DerefType>
    concept DereferencesTo = requires(T ptr) {
        {*ptr} -> std::convertible_to<DerefType>;
    };

    template<typename T>
    concept NamedInputStream = requires(T v) {
        { v } -> std::convertible_to<std::istream&>;
        { v.name() } -> std::convertible_to<std::string_view>;
    };

    template<typename T>
    concept AssociativeContainer = requires(T v) {
        typename T::key_type;
        typename T::mapped_type;
        typename T::value_type;
        { std::declval<typename T::value_type>().second } -> std::convertible_to<typename T::mapped_type>;
        { v.begin() } -> std::forward_iterator;
        { v.end() } -> std::forward_iterator;
        { v.find(std::declval<typename T::key_type>()) } -> std::same_as<typename T::iterator>;
    };

    template<typename Key, typename Container>
    concept AssociativeContainerKey = requires(Container& c, Key key) {
        requires AssociativeContainer<Container>;
        { c.find(key) } -> std::same_as<std::ranges::iterator_t<Container>>;
    };
}
