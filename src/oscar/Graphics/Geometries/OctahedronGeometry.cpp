#include "OctahedronGeometry.h"

#include <oscar/Graphics/Geometries/PolyhedronGeometry.h>
#include <oscar/Graphics/Mesh.h>
#include <oscar/Maths/GeometricFunctions.h>

#include <array>
#include <cstddef>
#include <cstdint>

using namespace osc;

osc::OctahedronGeometry::OctahedronGeometry(float radius, size_t detail)
{
    // implementation ported from threejs (OctahedronGeometry)

    auto const vertices = std::to_array<Vec3>({
        {1.0f,  0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f,  0.0f},
        {0.0f, -1.0f, 0.0f}, { 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, -1.0f},
    });

    auto const indices = std::to_array<uint32_t>({
        0, 2, 4,    0, 4, 3,    0, 3, 5,
        0, 5, 2,    1, 2, 5,    1, 5, 3,
        1, 3, 4,    1, 4, 2
    });

    m_Mesh = PolyhedronGeometry(vertices, indices, radius, detail);
}
