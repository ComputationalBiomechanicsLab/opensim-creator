#pragma once

#include "oscar/Graphics/TextureChannelFormat.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>

namespace osc
{
    enum class TextureFormat {
        RGBA32 = 0,
        RGB24,
        R8,
        RGBAFloat,
        NUM_OPTIONS,
    };

    size_t NumChannels(TextureFormat) noexcept;
    TextureChannelFormat ChannelFormat(TextureFormat) noexcept;
    size_t NumBytesPerPixel(TextureFormat) noexcept;
    std::optional<TextureFormat> ToTextureFormat(size_t numChannels, TextureChannelFormat) noexcept;
}
