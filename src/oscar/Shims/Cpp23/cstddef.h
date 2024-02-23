#pragma once

#include <cstddef>
#include <type_traits>

namespace osc::literals
{
    constexpr typename std::make_signed_t<size_t> operator""_z(unsigned long long v)
    {
        return static_cast<std::make_signed_t<size_t>>(v);
    }

    constexpr size_t operator""_zu(unsigned long long v)
    {
        return static_cast<size_t>(v);
    }
}
