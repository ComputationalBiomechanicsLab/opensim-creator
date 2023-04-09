#pragma once

#include <cstdint>
#include <optional>

namespace osc
{
    enum class TextureFormat : int32_t {
        RGBA32 = 0,
        RGB24,
        R8,
        TOTAL
    };

    std::optional<TextureFormat> NumChannelsAsTextureFormat(int32_t numChannels) noexcept;
}
