#pragma once

#include <oscar/Utils/Flags.h>

#include <cstdint>

namespace osc
{
    enum class CameraClearFlag : uint8_t {
        Nothing    = 0,
        SolidColor = 1<<0,
        Depth      = 1<<1,

        All = SolidColor | Depth,
        Default = SolidColor | Depth,
    };

    using CameraClearFlags = Flags<CameraClearFlag>;
}
