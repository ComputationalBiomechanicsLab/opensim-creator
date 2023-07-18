#pragma once

#include <cstdint>
#include <iosfwd>

// note: implementation is in `GraphicsImplementation.cpp`
namespace osc
{
    // how texels should be sampled when a texture coordinate falls outside the texture's bounds
    enum class TextureWrapMode : int32_t {
        Repeat = 0,
        Clamp,
        Mirror,
        TOTAL,
    };

    std::ostream& operator<<(std::ostream&, TextureWrapMode);
}
