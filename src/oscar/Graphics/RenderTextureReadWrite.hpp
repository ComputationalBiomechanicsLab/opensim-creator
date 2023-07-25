#pragma once

namespace osc
{
    enum class RenderTextureReadWrite {
        // render texture contains sRGB data, perform linear <--> sRGB when loading
        // textels in a shader
        sRGB = 0,

        // render texture contains linear data, don't perform any conversions when
        // loading textels in a shader
        Linear,

        NUM_OPTIONS,

        Default = sRGB,
    };
}
