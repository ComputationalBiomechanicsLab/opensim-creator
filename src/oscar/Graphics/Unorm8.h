#pragma once

#include <oscar/Graphics/Unorm.h>
#include <oscar/Utils/Conversion.h>

#include <cstddef>
#include <cstdint>

namespace osc
{
    // A normalized unsigned 8-bit integer that can be used to store a floating-point
    // number in the (clamped) range [0.0f, 1.0f]
    //
    // see: https://www.khronos.org/opengl/wiki/Normalized_Integer
    using Unorm8 = Unorm<uint8_t>;

    template<>
    struct Converter<std::byte, Unorm8> final {
        constexpr Unorm8 operator()(std::byte raw_value) const
        {
            return Unorm8{static_cast<uint8_t>(raw_value)};
        }
    };

    template<>
    struct Converter<Unorm8, std::byte> final {
        constexpr std::byte operator()(Unorm8 unorm) const
        {
            return static_cast<std::byte>(unorm.raw_value());
        }
    };
}
