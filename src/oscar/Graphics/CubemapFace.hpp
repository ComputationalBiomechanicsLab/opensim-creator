#pragma once

#include <cstdint>

namespace osc
{
    enum class CubemapFace : int32_t {
        PositiveX = 0,
        NegativeX,
        PositiveY,
        NegativeY,
        PositiveZ,
        NegativeZ,
        NUM_OPTIONS,
    };
}
