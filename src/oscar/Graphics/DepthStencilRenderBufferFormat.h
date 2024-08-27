#pragma once

#include <iosfwd>

namespace osc
{
    enum class DepthStencilRenderBufferFormat {

        // two-component, 32-bit packed, with 24 unsigned normalized bits for
        // the depth (D) component and 8 unsigned integer bits for the stencil (S)
        // component
        D24_UNorm_S8_UInt,

        // 32-bit signed floating point format for the depth (D) component
        D32_SFloat,

        NUM_OPTIONS,

        Default = D24_UNorm_S8_UInt,
    };

    std::ostream& operator<<(std::ostream&, DepthStencilRenderBufferFormat);
}
