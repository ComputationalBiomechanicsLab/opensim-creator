#pragma once

#include <iosfwd>

// note: implementation is in `Renderer.cpp`
namespace osc
{
    // format of the "color" (albedo) part of a render
    enum class RenderTextureFormat {
        ARGB32 = 0,
        RED,
        TOTAL,
    };

    std::ostream& operator<<(std::ostream&, RenderTextureFormat);
}