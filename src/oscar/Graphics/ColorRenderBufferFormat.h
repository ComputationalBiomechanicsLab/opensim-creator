#pragma once

#include <iosfwd>

namespace osc
{
    // the underlying format of a color render buffer
    //
    // note: the naming convention and docuemntation for each format is designed
    //       to be the same as Vulkan's:
    //
    //           https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkFormat.html
    enum class ColorRenderBufferFormat {

        // one-component, 8-bit unsigned normalized format that has a single 8-bit R component
        R8_UNORM,

        // a four-component, 32-bit unsigned normalized format that has an 8-bit R component in
        // byte 0, an 8-bit G component in byte 1, an 8-bit B component in byte 2, and an 8-bit A
        // component in byte 3.
        R8G8B8A8_UNORM,

        // a two-component, 32-bit signed floating-point format that has a 16-bit R component in
        // bytes 0..1, and a 16-bit G component in bytes 2..3
        R16G16_SFLOAT,

        // a three-component, 48-bit signed floating-point format that has a 16-bit R component in
        // bytes 0..1, a 16-bit G component in bytes 2..3, and a 16-bit B component in bytes 4..5
        R16G16B16_SFLOAT,

        // a four-component, 64-bit signed floating-point format that has a 16-bit R component in
        // bytes 0..1, a 16-bit G component in bytes 2..3, a 16-bit B component in bytes 4..5, and
        // a 16-bit A component in bytes 6..7
        R16G16B16A16_SFLOAT,

        // a one-component, 32-bit signed floating-point format that has a single 32-bit R component.
        R32_SFLOAT,

        NUM_OPTIONS,

        Default = R8G8B8A8_UNORM,
        DefaultHDR = R16G16B16A16_SFLOAT,
    };

    std::ostream& operator<<(std::ostream&, ColorRenderBufferFormat);
}
