#pragma once

#include <oscar/Shims/Cpp23/utility.hpp>

#include <cstdint>

namespace osc
{
    enum class ImageLoadingFlags : uint32_t {
        None = 0u,

        // BEWARE: this flips pixels vertically (in Y) but leaves the pixel's
        // contents untouched. This is fine if the pixels represent colors,
        // but can cause surprising behavior if the pixels represent vectors
        //
        // therefore, if you are flipping (e.g.) normal maps, you may *also* need
        // to flip the pixel content appropriately (e.g. if RGB represents XYZ then
        // you'll need to negate each G)
        FlipVertically = 1u<<0u,
    };

    constexpr bool operator&(ImageLoadingFlags a, ImageLoadingFlags b) noexcept
    {
        return osc::to_underlying(a) & osc::to_underlying(b);
    }
}
