#pragma once

#include <oscar/Graphics/Mesh.h>
#include <oscar/Maths/Angle.h>

#include <cstddef>

namespace osc
{
    class SphereGeometry final {
    public:
        static Mesh generate_mesh(
            float radius = 1.0f,
            size_t widthSegments = 32,
            size_t heightSegments = 16,
            Radians phiStart = Degrees{0},
            Radians phiLength = Degrees{360},
            Radians thetaStart = Degrees{0},
            Radians thetaLength = Degrees{180}
        );
    };
}
