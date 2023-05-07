#pragma once

#include <cstdint>
#include <cstddef>
#include <optional>

namespace osc
{
    enum class TextureFormat : int32_t {
        // uint8_t channels
        RGBA32 = 0,
        RGB24,
        R8,

        // float channels
        RGBAFloat,

        TOTAL
    };

    constexpr size_t NumTextureFormats() noexcept
    {
        return static_cast<size_t>(TextureFormat::TOTAL);
    }
    size_t NumChannels(TextureFormat) noexcept;
    size_t NumBytesPerChannel(TextureFormat) noexcept;
    size_t NumBytesPerPixel(TextureFormat) noexcept;

    template<typename T> std::optional<TextureFormat> ToTextureFormat(size_t numChannels) noexcept;
    template<> std::optional<TextureFormat> ToTextureFormat<uint8_t>(size_t numChannels) noexcept;
    template<> std::optional<TextureFormat> ToTextureFormat<float>(size_t numChannels) noexcept;
}
