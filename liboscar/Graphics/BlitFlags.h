#pragma once

#include <liboscar/Utils/Flags.h>

namespace osc
{
    enum class BlitFlag {
        None       = 0,
        AlphaBlend = 1<<0,
    };

    using BlitFlags = Flags<BlitFlag>;
}
