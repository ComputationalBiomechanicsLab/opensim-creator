#pragma once

#include <cstdint>
#include <iosfwd>

// note: implementation is in `GraphicsImplementation.cpp`
namespace osc
{
    // format of the "color" (albedo) part of a render
    enum class RenderTextureFormat : int32_t {
        ARGB32 = 0,
        RED,
        TOTAL,
    };

    std::ostream& operator<<(std::ostream&, RenderTextureFormat);
}