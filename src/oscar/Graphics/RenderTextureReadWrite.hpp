#pragma once

namespace osc
{
    // enum that dictates the color conversion mode of a `RenderTexture`
    enum class RenderTextureReadWrite {
        // render texture contains sRGB data, perform linear <--> sRGB
        sRGB = 0,

        // render texture contains linear data, don't perform any conversions
        Linear,

        TOTAL,
        Default = sRGB,
    };
}