#pragma once

namespace osc
{
    // The color space conversion mode of a render buffer
    //
    // In oscar's graphics backend, shaders are always assumed to use and produce
    // linear colors. The shader may need to perform sRGB-to-linear conversion when:
    //
    // - Reading (sampling) the render buffer, e.g. when the resolved render buffer's
    //   texture is sampled in a downstream shader
    //
    // - Writing (rendering to) the render buffer, e.g. when a fragment shader writes a
    //   (linear) fragment to an attached render buffer
    //
    // If a shader encodes non-colors with its fragment shader (e.g. if it writes normal
    // vectors in the RGB channels) then this automatic conversion is undesirable. In
    // this case, you should set `RenderTextureReadWrite`  to `Linear`, which tells
    // the backend to skip colorspace conversion.
    //
    // This enum only has an effect when using non-`_SFLOAT` render buffer formats, because
    // floating-point formats have enough precision to directly store linear colors
    // with no conversion.
    //
    // For graphics developers, if you're wondering "why don't you just merge this
    // into `ColorRenderBufferFormat` as `ARGB8_SRGB`?" it's because different native
    // graphics backends have different degrees of native sRGB conversion support. For
    // example, OpenGL3 only provides `GL_SRGB8` and `GL_SRGB8_ALPHA8`, whereas Vulkan
    // provides 20+ sRGB formats (e.g. `VK_FORMAT_R8_SRGB`, `VK_FORMAT_R8G8_SRGB`, ...).
    // So the conversion
    enum class RenderBufferReadWrite {
        // render texture contains sRGB data, perform linear <--> sRGB when loading
        // textels in a shader
        sRGB,

        // render texture contains linear data, don't perform any conversions when
        // loading textels in a shader
        Linear,

        NUM_OPTIONS,
        Default = sRGB,
    };
}
