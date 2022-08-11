#pragma once

#include <iosfwd>

// note: implementation is in `GraphicsImplementation.cpp`
namespace osc
{
    // how texels should be sampled when a texture coordinate falls outside the texture's bounds
    enum class TextureWrapMode {
        Repeat = 0,
        Clamp,
        Mirror,
        TOTAL,
    };

    std::ostream& operator<<(std::ostream&, TextureWrapMode);
}