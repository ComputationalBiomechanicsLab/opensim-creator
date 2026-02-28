#pragma once

#include <liboscar/maths/vector.h>

namespace osc { struct PolarPerspectiveCamera; }

namespace osc
{
    class CameraViewAxes final {
    public:
        Vector2 dimensions() const;
        bool draw(PolarPerspectiveCamera&);
    };
}
