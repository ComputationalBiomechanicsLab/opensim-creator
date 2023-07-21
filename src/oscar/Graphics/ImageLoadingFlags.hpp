#pragma once

#include <cstdint>
#include <type_traits>

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
        using T = std::underlying_type_t<ImageLoadingFlags>;
        return static_cast<T>(a) & static_cast<T>(b);
    }
}
