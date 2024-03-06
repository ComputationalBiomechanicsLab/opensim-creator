#include "ConeGeometry.h"

#include <oscar/Graphics/Geometries/CylinderGeometry.h>
#include <oscar/Graphics/Mesh.h>
#include <oscar/Maths/Angle.h>

#include <cstddef>

using namespace osc;

Mesh osc::ConeGeometry::generate_mesh(
    float radius,
    float height,
    size_t radialSegments,
    size_t heightSegments,
    bool openEnded,
    Radians thetaStart,
    Radians thetaLength)
{
    return CylinderGeometry::generate_mesh(
        0.0f,
        radius,
        height,
        radialSegments,
        heightSegments,
        openEnded,
        thetaStart,
        thetaLength
    );
}
