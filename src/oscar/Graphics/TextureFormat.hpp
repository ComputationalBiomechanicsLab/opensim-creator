#pragma once

#include <oscar/Graphics/TextureChannelFormat.hpp>

#include <cstddef>
#include <optional>

namespace osc
{
    enum class TextureFormat {
        RGBA32,
        RGB24,
        R8,
        RGBFloat,
        RGBAFloat,

        NUM_OPTIONS,
    };

    size_t NumChannels(TextureFormat);
    TextureChannelFormat ChannelFormat(TextureFormat);
    size_t NumBytesPerPixel(TextureFormat);
    std::optional<TextureFormat> ToTextureFormat(size_t numChannels, TextureChannelFormat);
}
