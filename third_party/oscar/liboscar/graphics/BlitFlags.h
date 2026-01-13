#pragma once

#include <liboscar/utils/Flags.h>

namespace osc
{
    enum class BlitFlag {
        None       = 0,
        AlphaBlend = 1<<0,
    };

    using BlitFlags = Flags<BlitFlag>;
}
