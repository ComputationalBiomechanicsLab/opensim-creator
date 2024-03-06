#pragma once

#include <oscar/Graphics/Mesh.h>
#include <oscar/Maths/Angle.h>

#include <cstddef>

namespace osc
{
    class CylinderGeometry final {
    public:
        static Mesh generate_mesh(
            float radiusTop = 1.0f,
            float radiusBottom = 1.0f,
            float height = 1.0f,
            size_t radialSegments = 32,
            size_t heightSegments = 1,
            bool openEnded = false,
            Radians thetaStart = Degrees{0},
            Radians thetaLength = Degrees{360}
        );
    };
}
