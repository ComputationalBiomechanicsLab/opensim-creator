#pragma once

#include <liboscar/maths/vector.h>

namespace osc { class Camera; }
namespace osc { struct OrbitCameraController; }

namespace osc
{
    class CameraViewAxes final {
    public:
        Vector2 dimensions() const;
        bool draw(OrbitCameraController&, const Camera&);
    };
}
