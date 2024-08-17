#pragma once

#include <cstdint>

namespace osc
{
    enum class DepthFunction : uint8_t {
        Less,
        LessOrEqual,
        NUM_OPTIONS,

        Default = Less,
    };
}
