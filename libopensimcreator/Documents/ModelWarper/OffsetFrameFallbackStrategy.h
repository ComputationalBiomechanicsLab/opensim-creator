#pragma once

namespace osc
{
    enum class OffsetFrameFallbackStrategy {
        Error,
        Ignore,
        WarpPosition,
        NUM_OPTIONS,
        Default = Error,
    };
}
