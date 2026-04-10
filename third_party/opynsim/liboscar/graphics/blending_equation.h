#pragma once

#include <cstdint>

namespace osc
{
    // when `Material::is_transparent()` is `true`, this is the equation that is
    // used by the graphics backend to blend the source fragment's color, which
    // is written by the `Shader` and scaled by `SourceBlendingFactor`, with
    // the destination fragment's color, which is present in the `RenderTarget`'s
    // `RenderTargetColorAttachment`s and scaled by `DestinationBlendingFactor`
    enum class BlendingEquation : uint8_t {
        Add,
        Min,
        Max,
        NUM_OPTIONS,

        Default = Add,
    };
}
