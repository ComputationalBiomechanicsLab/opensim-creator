#pragma once

#include <cstdint>

namespace osc
{
    enum class CameraClearFlags : int32_t {
        SolidColor = 0,
        Depth,
        Nothing,
        TOTAL,
        Default = SolidColor,
    };
}
