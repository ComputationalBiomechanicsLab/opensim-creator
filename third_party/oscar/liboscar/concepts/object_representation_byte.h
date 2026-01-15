#pragma once

#include <liboscar/concepts/same_as_any_of.h>

#include <cstddef>

namespace osc
{
    // Satisfied if `T` is a a valid representation of one byte of a
    // C++ object.
    //
    // > see: https://en.cppreference.com/w/cpp/language/object#Object_representation_and_value_representation
    template<typename T>
    concept ObjectRepresentationByte = SameAsAnyOf<T, unsigned char, std::byte>;
}
