#pragma once

#include <iosfwd>

namespace osc
{
    // the underlying format of a depth render buffer
    //
    // note: the naming convention and docuemntation for each format is designed
    //       to be the same as Vulkan's:
    //
    //           https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkFormat.html
    enum class DepthStencilRenderBufferFormat {

        // two-component, 32-bit packed format that has 8 unsigned integer bits
        // in the stencil component and 24 unsigned normalized bits in the depth
        // component
        D24_UNorm_S8_UInt,

        // one-component, 32-bit signed floating-point format that has 32 bits in
        // the depth component
        D32_SFloat,

        NUM_OPTIONS,

        Default = D24_UNorm_S8_UInt,
    };

    std::ostream& operator<<(std::ostream&, DepthStencilRenderBufferFormat);
}
