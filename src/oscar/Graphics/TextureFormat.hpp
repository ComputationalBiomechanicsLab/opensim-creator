#pragma once

#include <cstddef>
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
    size_t NumBytesPerChannel(TextureFormat) noexcept;
    size_t NumBytesPerPixel(TextureFormat) noexcept;

    template<typename T> std::optional<TextureFormat> ToTextureFormat(size_t numChannels) noexcept;
    template<> std::optional<TextureFormat> ToTextureFormat<uint8_t>(size_t numChannels) noexcept;
    template<> std::optional<TextureFormat> ToTextureFormat<float>(size_t numChannels) noexcept;
}
