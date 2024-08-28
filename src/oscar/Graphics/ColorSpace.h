#pragma once


namespace osc
{
    // Used to indicate the encoding used when converting the color components of
    // a texture between color spaces.
    //
    // It's necessary to provide this as additional information when implementations
    // can't discern the color encoding of the color components they're handling, but
    // need to discern it for conversion purposes.
    //
    // For example, image loaders might be able to decode a bitstream into red, green,
    // and blue color components, but the bitstream may not contain enough metadata
    // to tell the image loader what the colorspace of those components are. If the
    // image then plans on using the image with the graphics backend (e.g. via `Texture2D`),
    // and the graphics backend "anchors" shaders into a linear color space, then the
    // loader needs to be able to tell the graphics backend whether (or not) to
    // perfrom (e.g.) sRGB-to-linear conversion when loading the color components
    // into a shader.
    enum class ColorSpace {

        // Indicates that the color channels are encoded in a linear color space. This
        // encoding typical for (e.g.) normal maps and other vector-encoded image formats.
        Linear,

        // Indicates that the color channels are encoded in a sRGB color space. This
        // implies that (e.g.) shaders need to perform sRGB <-> linear conversion when
        // sampling from, or rendering to, the texture. This encoding is typical for
        // (e.g.) albedo textures.
        sRGB,

        NUM_OPTIONS,
    };
}
