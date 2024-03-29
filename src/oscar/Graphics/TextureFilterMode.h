#pragma once

#include <iosfwd>

namespace osc
{
    // how sampling should handle when the sampling location falls between multiple textels
    enum class TextureFilterMode {
        Nearest,
        Linear,
        Mipmap,  // linear when magnifying, though
        NUM_OPTIONS,
    };

    std::ostream& operator<<(std::ostream&, TextureFilterMode);
}
