#pragma once

#include <iosfwd>

// note: implementation is in `GraphicsImplementation.cpp`
namespace osc
{
    // the shape of the viewing frustrum that the camera uses
    enum class CameraProjection {
        Perspective,
        Orthographic,
        NUM_OPTIONS,
    };

    std::ostream& operator<<(std::ostream&, CameraProjection);
}
