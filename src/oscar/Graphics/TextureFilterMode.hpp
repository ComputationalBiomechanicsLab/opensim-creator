#pragma once

#include <cstdint>
#include <iosfwd>

// note: implementation is in `GraphicsImplementation.cpp`
namespace osc
{
    // how sampling should handle when the sampling location falls between multiple textels
    enum class TextureFilterMode : int32_t {
        Nearest = 0,
        Linear,
        Mipmap,
        TOTAL,
    };

    std::ostream& operator<<(std::ostream&, TextureFilterMode);
}
