#pragma once

#include <liboscar/Graphics/Unorm8.h>
#include <liboscar/Graphics/Rgba.h>
#include <liboscar/Shims/Cpp20/bit.h>

#include <concepts>

namespace osc
{
    using Color32 = Rgba<Unorm8>;

    template<std::integral T>
    T to_integer(const Color32& color32)
    {
        return cpp20::bit_cast<T>(color32);
    }
}

