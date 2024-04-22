#pragma once

#include <oscar/Graphics/TextureChannelFormat.h>

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

    size_t num_channels_in(TextureFormat);
    TextureChannelFormat channel_format_of(TextureFormat);
    size_t num_bytes_per_pixel_in(TextureFormat);
    std::optional<TextureFormat> to_texture_format(size_t num_channels, TextureChannelFormat);
}
