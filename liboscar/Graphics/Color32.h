#pragma once

#include <liboscar/Graphics/Rgba.h>
#include <liboscar/Graphics/Unorm8.h>

#include <bit>
#include <concepts>

namespace osc
{
    using Color32 = Rgba<Unorm8>;

    template<std::integral T>
    constexpr T to_integer(const Color32& color32)
    {
        return std::bit_cast<T>(color32);
    }
}

