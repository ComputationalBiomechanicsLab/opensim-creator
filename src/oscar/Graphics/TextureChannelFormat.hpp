#pragma once

#include <cstddef>

namespace osc
{
    enum class TextureChannelFormat {
        Uint8,
        Float32,
        NUM_OPTIONS,
    };

    size_t NumBytesPerChannel(TextureChannelFormat);
}
