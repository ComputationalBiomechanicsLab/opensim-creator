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
        NUM_OPTIONS,
    };

    std::ostream& operator<<(std::ostream&, TextureWrapMode);
}
