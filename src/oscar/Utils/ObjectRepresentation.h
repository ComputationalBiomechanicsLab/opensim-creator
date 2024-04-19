#pragma once

#include <oscar/Utils/Concepts.h>

#include <cstddef>
#include <ranges>
#include <span>

namespace osc
{
    template<ObjectRepresentationByte Byte = std::byte, BitCastable T>
    constexpr std::span<const Byte> view_object_representation(const T& v)
    {
        // this is one of the few cases where `reinterpret_cast` is guaranteed to be safe
        // for _examination_ (i.e. reading)
        //
        // > from: https://en.cppreference.com/w/cpp/language/reinterpret_cast
        // >
        // > Whenever an attempt is made to read or modify the stored value of an object of
        // > type DynamicType through a glvalue of type AliasedType, the behavior is undefined
        // > unless one of the following is true:
        // >
        // > - AliasedType is std::byte,(since C++17) char, or unsigned char: this permits
        // >   examination of the object representation of any object as an array of bytes.
        return {reinterpret_cast<const Byte*>(&v), sizeof(T)};
    }

    template<ObjectRepresentationByte Byte = std::byte, std::ranges::contiguous_range Range>
    requires BitCastable<typename Range::value_type>
    constexpr std::span<const Byte> view_object_representations(const Range& range)
    {
        // this is one of the few cases where `reinterpret_cast` is guaranteed to be safe
        // for _examination_ (i.e. reading)
        //
        // > from: https://en.cppreference.com/w/cpp/language/reinterpret_cast
        // >
        // > Whenever an attempt is made to read or modify the stored value of an object of
        // > type DynamicType through a glvalue of type AliasedType, the behavior is undefined
        // > unless one of the following is true:
        // >
        // > - AliasedType is std::byte,(since C++17) char, or unsigned char: this permits
        // >   examination of the object representation of any object as an array of bytes.
        return {reinterpret_cast<const Byte*>(std::ranges::data(range)), sizeof(typename Range::value_type) * std::ranges::size(range)};
    }
}
