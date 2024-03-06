#include "IcosahedronGeometry.h"

#include <oscar/Graphics/Geometries/PolyhedronGeometry.h>
#include <oscar/Graphics/Mesh.h>
#include <oscar/Maths/GeometricFunctions.h>

#include <array>
#include <cstddef>
#include <cstdint>

using namespace osc;

Mesh osc::IcosahedronGeometry::generate_mesh(float radius, size_t detail)
{
    float const t = (1.0f + sqrt(5.0f))/2.0f;

    auto const vertices = std::to_array<Vec3>({
        {-1.0f,  t,     0.0f}, {1.0f, t,    0.0f}, {-1.0f, -t,     0.0f}, { 1.0f, -t,     0.0f},
        { 0.0f, -1.0f,  t   }, {0.0f, 1.0f, t   }, { 0.0f, -1.0f, -t   }, { 0.0f,  1.0f, -t   },
        { t,     0.0f, -1.0f}, {t,    0.0f, 1.0f}, {-t,     0.0f, -1.0f}, {-t,     0.0f,  1.0f},
    });

    auto const indices = std::to_array<uint32_t>({
        0, 11, 5,    0, 5,  1,     0,  1,  7,     0,  7, 10,    0, 10, 11,
        1, 5,  9,    5, 11, 4,     11, 10, 2,     10, 7, 6,     7, 1,  8,
        3, 9,  4,    3, 4,  2,     3,  2,  6,     3,  6, 8,     3, 8, 9,
        4, 9,  5,    2, 4,  11,    6,  2,  10,    8,  6, 7,     9, 8, 1,
    });

    return PolyhedronGeometry::generate_mesh(vertices, indices, radius, detail);
}
