#pragma once

#include <type_traits>

namespace osc
{
    // Satisfied if `T` is bit-castable and, therefore, can be used with `std::bit_cast`.
    //
    // > "For TriviallyCopyable types, value representation is a part of the object representation"
    // >
    // > source: https://en.cppreference.com/w/cpp/language/object#Object_representation_and_value_representation
    template<typename T>
    concept BitCastable = std::is_trivially_copyable_v<T> and std::is_trivially_destructible_v<T>;
}
