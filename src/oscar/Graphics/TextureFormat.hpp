#pragma once

#include <oscar/Graphics/TextureChannelFormat.hpp>

#include <cstddef>
#include <optional>

namespace osc
{
    enum class TextureFormat {
        R8,
        RG16,
        RGB24,
        RGBA32,

        RGFloat,
        RGBFloat,
        RGBAFloat,

        NUM_OPTIONS,
    };

    size_t NumChannels(TextureFormat);
    TextureChannelFormat ChannelFormat(TextureFormat);
    size_t NumBytesPerPixel(TextureFormat);
    std::optional<TextureFormat> ToTextureFormat(size_t numChannels, TextureChannelFormat);
}
