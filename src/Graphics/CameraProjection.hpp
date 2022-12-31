#pragma once

#include <cstdint>
#include <iosfwd>

// note: implementation is in `GraphicsImplementation.cpp`
namespace osc
{
    // the shape of the viewing frustrum that the camera uses
    enum class CameraProjection : int32_t {
        Perspective = 0,
        Orthographic,
        TOTAL,
    };

    std::ostream& operator<<(std::ostream&, CameraProjection);
}