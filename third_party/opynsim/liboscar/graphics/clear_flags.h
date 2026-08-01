#pragma once

#include <liboscar/utilities/flags.h>

#include <cstdint>

namespace osc
{
    /// Represents a single flag that the renderer uses when deciding
    /// whether to clear part of a render target.
    enum class ClearFlag : uint8_t {
        None       = 0,
        SolidColor = 1<<0,
        Depth      = 1<<1,

        All = SolidColor | Depth,
        Default = SolidColor | Depth,
    };

    /// Represents flags that the renderer uses when deciding which
    /// parts of a render target to clear.
    using ClearFlags = Flags<ClearFlag>;
}
