#include "TorusGeometry.h"

#include <oscar/Graphics/Mesh.h>
#include <oscar/Maths/Angle.h>
#include <oscar/Maths/TrigonometricFunctions.h>
#include <oscar/Maths/UnitVec3.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/Maths/Vec3.h>

#include <cstddef>
#include <cstdint>
#include <vector>

using namespace osc;
using namespace osc::literals;

osc::TorusGeometry::TorusGeometry(
    float radius,
    float tube,
    size_t radialSegments,
    size_t tubularSegments,
    Radians arc)
{
    // (ported from three.js/TorusGeometry)

    const auto fradialSegments = static_cast<float>(radialSegments);
    const auto ftubularSegments = static_cast<float>(tubularSegments);

    std::vector<uint32_t> indices;
    std::vector<Vec3>  vertices;
    std::vector<Vec3> normals;
    std::vector<Vec2> uvs;

    for (size_t j = 0; j <= radialSegments; ++j) {
        const auto fj = static_cast<float>(j);
        for (size_t i = 0; i <= tubularSegments; ++i) {
            const auto fi = static_cast<float>(i);
            const Radians u = fi/ftubularSegments * arc;
            const Radians v = fj/fradialSegments * 360_deg;

            const Vec3& vertex = vertices.emplace_back(
                (radius + tube * cos(v)) * cos(u),
                (radius + tube * cos(v)) * sin(u),
                tube * sin(v)
            );
            normals.push_back(UnitVec3{
                vertex.x - radius*cos(u),
                vertex.y - radius*sin(u),
                vertex.z - 0.0f,
            });
            uvs.emplace_back(fi/ftubularSegments, fj/fradialSegments);
        }
    }

    for (size_t j = 1; j <= radialSegments; ++j) {
        for (size_t i = 1; i <= tubularSegments; ++i) {
            const auto a = static_cast<uint32_t>((tubularSegments + 1)*(j + 0) + i - 1);
            const auto b = static_cast<uint32_t>((tubularSegments + 1)*(j - 1) + i - 1);
            const auto c = static_cast<uint32_t>((tubularSegments + 1)*(j - 1) + i);
            const auto d = static_cast<uint32_t>((tubularSegments + 1)*(j + 0) + i);

            indices.insert(indices.end(), {a, b, d});
            indices.insert(indices.end(), {b, c, d});
        }
    }

    m_Mesh.setVerts(vertices);
    m_Mesh.setNormals(normals);
    m_Mesh.setTexCoords(uvs);
    m_Mesh.setIndices(indices);
}
