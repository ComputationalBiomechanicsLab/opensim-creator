#pragma once

#include <liboscar/graphics/Rgba.h>
#include <liboscar/graphics/Unorm8.h>

#include <bit>
#include <concepts>
#include <cstdint>

namespace osc
{
    using Color32 = Rgba<Unorm8>;

    template<std::integral T>
    constexpr T to_integer(const Color32& color32)
    {
        return std::bit_cast<T>(color32);
    }
}

// specialized hashing function for `Color32`
template<>
struct std::hash<osc::Color32> final {
    size_t operator()(const osc::Color32& color32) const noexcept
    {
        return std::hash<uint32_t>{}(std::bit_cast<uint32_t>(color32));
    }
};
