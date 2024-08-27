#pragma once

#include <iosfwd>

namespace osc
{
    // format of the "color" (albedo) part of a render
    enum class ColorRenderBufferFormat {
        Red8,  // careful: `Red8` has previously caused an explosion on some Intel machines (#418)
        ARGB32,

        RGFloat16,
        RGBFloat16,
        ARGBFloat16,

        Depth,  // implementation-defined: pretend it's a high-res red texture
        NUM_OPTIONS,

        Default = ARGB32,
        DefaultHDR = ARGBFloat16,
    };

    std::ostream& operator<<(std::ostream&, ColorRenderBufferFormat);
}
