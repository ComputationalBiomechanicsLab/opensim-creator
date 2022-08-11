#pragma once

// note: implementation is in `GraphicsImplementation.cpp`
namespace osc
{
    // how sampling should handle when the sampling location falls between multiple textels
    enum class TextureFilterMode {
        Nearest = 0,
        Linear,
        Mipmap,
        TOTAL,
    };
    std::ostream& operator<<(std::ostream&, TextureFilterMode);
}