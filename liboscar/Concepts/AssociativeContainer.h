#pragma once

#include <concepts>
#include <iterator>
#include <utility>

namespace osc
{
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
}
