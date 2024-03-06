#pragma once

#include <oscar/Graphics/Mesh.h>
#include <oscar/Maths/Angle.h>

#include <cstddef>

namespace osc
{
    class RingGeometry final {
    public:
        static Mesh generate_mesh(
            float innerRadius = 0.5f,
            float outerRadius = 1.0f,
            size_t thetaSegments = 32,
            size_t phiSegments = 1,
            Radians thetaStart = Degrees{0},
            Radians thetaLength = Degrees{360}
        );
    };
}
