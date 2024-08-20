#pragma once

#include <cstdint>

namespace osc
{
    enum class BlendFunction : uint8_t {
        One,
        Zero,
        SourceAlpha,
        OneMinusSourceAlpha,
        NUM_OPTIONS,

        // defaults currently assume blending with associated (in contrast to
        // premultiplied) alpha via `BlendEquation::Add`
        SourceDefault = SourceAlpha,
        DestinationDefault = OneMinusSourceAlpha,
    };
}
