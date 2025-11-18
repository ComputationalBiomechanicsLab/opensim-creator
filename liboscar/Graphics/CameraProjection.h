#pragma once

#include <iosfwd>

namespace osc
{
    // the shape of the viewing frustum that the camera uses
    enum class CameraProjection {
        Perspective,
        Orthographic,
        NUM_OPTIONS,

        Default = Perspective,
    };

    std::ostream& operator<<(std::ostream&, CameraProjection);
}
