#pragma once

namespace osc
{
    // Used in various parts of the codebase (e.g. image loaders) to indicate the
    // color space of the provided pixel data.
    //
    // This is necessary because some functions are only capable of knowing that they
    // are loading pixel data (e.g. as RGB bytes), but they aren't capable of knowing
    // the color space encoding of that data (e.g. when loading a normal map from a
    // PNG). It's important metadata, because shaders always work in a linear color space,
    // and other calling code may only want to deal with sRGB colors (e.g. even if
    // the colors were originally loaded as linear-space floats)
    enum class ColorSpace {

        // Indicates an sRGB colorspace. This is typical for (e.g.) image files, albedo textures
        sRGB,

        // Indicates a linear colorspace. This is typical for (e.g.) bump maps, normal maps
        Linear,

        NUM_OPTIONS,
    };
}
