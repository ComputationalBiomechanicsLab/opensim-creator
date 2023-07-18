#pragma once

#include <cstdint>
#include <iosfwd>

// note: implementation is in `GraphicsImplementation.cpp`
namespace osc
{
    enum class DepthStencilFormat : int32_t {
        D24_UNorm_S8_UInt = 0,
        TOTAL,
    };

    std::ostream& operator<<(std::ostream&, DepthStencilFormat);
}
