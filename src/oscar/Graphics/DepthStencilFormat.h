#pragma once

#include <iosfwd>

namespace osc
{
    enum class DepthStencilFormat {
        D24_UNorm_S8_UInt,
        NUM_OPTIONS,
    };

    std::ostream& operator<<(std::ostream&, DepthStencilFormat);
}
