#pragma once

#include <liboscar/Concepts/IsAnyOf.h>

#include <cstddef>

namespace osc
{
    // see: https://en.cppreference.com/w/cpp/language/object#Object_representation_and_value_representation
    template<typename T>
    concept ObjectRepresentationByte = IsAnyOf<T, unsigned char, std::byte>;
}
