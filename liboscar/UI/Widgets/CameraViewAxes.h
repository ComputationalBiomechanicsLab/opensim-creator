#pragma once

#include <liboscar/Maths/Vec2.h>

namespace osc { struct PolarPerspectiveCamera; }

namespace osc
{
    class CameraViewAxes final {
    public:
        Vec2 dimensions() const;
        bool draw(PolarPerspectiveCamera&);
    };
}
