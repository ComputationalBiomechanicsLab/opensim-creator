#include "TetrahedronGeometry.h"

#include <oscar/Graphics/Geometries/PolyhedronGeometry.h>
#include <oscar/Graphics/Mesh.h>
#include <oscar/Maths/GeometricFunctions.h>

#include <array>
#include <cstddef>
#include <cstdint>

using namespace osc;

osc::TetrahedronGeometry::TetrahedronGeometry(float radius, size_t detail_level)
{
    // implementation ported from threejs (TetrahedronGeometry)

    const auto vertices = std::to_array<Vec3>({
        {1, 1, 1}, {-1, -1, 1}, {-1, 1, - 1}, {1, -1, -1},
    });

    const auto indices = std::to_array<uint32_t>({
        2, 1, 0,    0, 3, 2,    1, 3, 0,    2, 3, 1
    });

    mesh_ = PolyhedronGeometry(vertices, indices, radius, detail_level);
}
