#pragma once

#include <cstdint>

namespace osc
{
    enum class BlendEquation : uint8_t {
        Add,
        Min,
        Max,
        NUM_OPTIONS,

        Default = Add,
    };
}
