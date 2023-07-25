#pragma once

#include <iosfwd>

// note: implementation is in `GraphicsImplementation.cpp`
namespace osc
{
    // format of the "color" (albedo) part of a render
    enum class RenderTextureFormat {
        ARGB32 = 0,
        RED,
        ARGBHalf,
        NUM_OPTIONS,

        Default = ARGB32,
        DefaultHDR = ARGBHalf,
    };

    std::ostream& operator<<(std::ostream&, RenderTextureFormat);
}
