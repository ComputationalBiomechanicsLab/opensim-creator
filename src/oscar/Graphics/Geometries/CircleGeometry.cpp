#include "CircleGeometry.h"

#include <oscar/Graphics/Mesh.h>
#include <oscar/Maths/Angle.h>
#include <oscar/Maths/TrigonometricFunctions.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/Shims/Cpp23/cstddef.h>

#include <cstddef>
#include <cstdint>
#include <vector>

using namespace osc;
using namespace osc::literals;

Mesh osc::CircleGeometry::generate_mesh(
    float radius,
    size_t segments,
    Radians thetaStart,
    Radians thetaLength)
{
    // this implementation was initially hand-ported from threejs (CircleGeometry)

    segments = max(3_uz, segments);
    auto const fsegments = static_cast<float>(segments);

    std::vector<uint32_t> indices;
    std::vector<Vec3> vertices;
    std::vector<Vec3> normals;
    std::vector<Vec2> uvs;

    // middle vertex
    vertices.emplace_back(0.0f, 0.0f, 0.0f);
    normals.emplace_back(0.0f, 0.0f, 1.0f);
    uvs.emplace_back(0.5f, 0.5f);

    // not-middle vertices
    for (ptrdiff_t s = 0; s <= static_cast<ptrdiff_t>(segments); ++s) {
        auto const fs = static_cast<float>(s);
        auto const segment = thetaStart + (fs/fsegments * thetaLength);
        auto const cosSeg = cos(segment);
        auto const sinSeg = sin(segment);

        vertices.emplace_back(radius * cosSeg, radius * sinSeg, 0.0f);
        normals.emplace_back(0.0f, 0.0f, 1.0f);
        uvs.emplace_back((cosSeg + 1.0f) / 2.0f, (sinSeg + 1.0f) / 2.0f);
    }

    for (uint32_t i = 1; i <= static_cast<uint32_t>(segments); ++i) {
        indices.insert(indices.end(), {i, i+1, 0});
    }

    Mesh rv;
    rv.setVerts(vertices);
    rv.setNormals(normals);
    rv.setTexCoords(uvs);
    rv.setIndices(indices);
    return rv;
}
