#pragma once

#include <type_traits>

namespace osc
{
    // see:
    //
    // - std::bit_cast (similar constraints)
    // - https://en.cppreference.com/w/cpp/language/object#Object_representation_and_value_representation
    //   > "For TriviallyCopyable types, value representation is a part of the object representation"
    template<typename T>
    concept BitCastable = std::is_trivially_copyable_v<T> && std::is_trivially_destructible_v<T>;
}
