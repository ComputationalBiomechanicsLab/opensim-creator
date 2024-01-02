#pragma once

namespace osc
{
    // used to tell the renderer the color space of provided pixel data
    //
    // this is necessary because OSC assumes that all color data within shaders
    // is within a linear color space, but the color space of external data (e.g.
    // loaded from images) will typically be in sRGB color space and require on-GPU
    // conversion
    enum class ColorSpace {
        // hint: typical image files (e.g. from photo editors, online, etc.) are defined in sRGB
        sRGB,

        // hint: data image files (e.g. bump maps, normal maps) are usually defined in a linear color space
        Linear,

        NUM_OPTIONS,
    };
}
