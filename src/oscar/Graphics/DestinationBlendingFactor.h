#pragma once

#include <cstdint>

namespace osc
{
    enum class DestinationBlendingFactor : uint8_t {
        One,
        Zero,
        SourceAlpha,
        OneMinusSourceAlpha,
        NUM_OPTIONS,

        Default = OneMinusSourceAlpha,
    };
}