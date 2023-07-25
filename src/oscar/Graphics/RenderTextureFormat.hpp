#pragma once

#include <iosfwd>

// note: implementation is in `GraphicsImplementation.cpp`
namespace osc
{
    // format of the "color" (albedo) part of a render
    enum class RenderTextureFormat {
        ARGB32 = 0,
        ARGBFloat16,
        Red8,
        Depth,  // implementation-defined: pretend it's a high-res red texture
        NUM_OPTIONS,

        Default = ARGB32,
        DefaultHDR = ARGBFloat16,
    };

    std::ostream& operator<<(std::ostream&, RenderTextureFormat);
}
