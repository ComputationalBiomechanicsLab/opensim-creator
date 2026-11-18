#pragma once

#include <liboscar/maths/vector.h>

namespace osc
{
    class Camera;
    struct OrbitCameraController;

    class CameraViewAxes final {
    public:
        Vector2 dimensions() const;
        bool draw(OrbitCameraController&, const Camera&);
    };
}
