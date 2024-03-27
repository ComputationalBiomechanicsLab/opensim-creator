#include "SphereGeometry.h"

#include <oscar/Graphics/Mesh.h>
#include <oscar/Shims/Cpp23/cstddef.h>
#include <oscar/Maths/Angle.h>
#include <oscar/Maths/CommonFunctions.h>
#include <oscar/Maths/TrigonometricFunctions.h>

#include <cstddef>
#include <cstdint>
#include <vector>

using namespace osc;
using namespace osc::literals;

osc::SphereGeometry::SphereGeometry(
    float radius,
    size_t widthSegments,
    size_t heightSegments,
    Radians phiStart,
    Radians phiLength,
    Radians thetaStart,
    Radians thetaLength)
{
    // implementation was initially hand-ported from three.js (SphereGeometry)

    widthSegments = max(3_uz, widthSegments);
    heightSegments = max(2_uz, heightSegments);
    const auto fwidthSegments = static_cast<float>(widthSegments);
    const auto fheightSegments = static_cast<float>(heightSegments);
    const auto thetaEnd = min(thetaStart + thetaLength, 180_deg);

    uint32_t index = 0;
    std::vector<std::vector<uint32_t>> grid;

    std::vector<uint32_t> indices;
    std::vector<Vec3> vertices;
    std::vector<Vec3> normals;
    std::vector<Vec2> uvs;

    // generate vertices, normals, and uvs
    for (size_t iy = 0; iy <= heightSegments; ++iy) {
        std::vector<uint32_t> verticesRow;
        const float v = static_cast<float>(iy) / fheightSegments;

        // edge-case: poles
        float uOffset = 0.0f;
        if (iy == 0 && thetaStart == 0_deg) {
            uOffset = 0.5f / fwidthSegments;
        }
        else if (iy == heightSegments && thetaEnd == 180_deg) {
            uOffset = -0.5f / fwidthSegments;
        }

        for (size_t ix = 0; ix <= widthSegments; ++ix) {
            const float u = static_cast<float>(ix) / fwidthSegments;

            const Vec3& vertex = vertices.emplace_back(
                -radius * cos(phiStart + u*phiLength) * sin(thetaStart + v*thetaLength),
                radius * cos(thetaStart + v*thetaLength),
                radius * sin(phiStart + u*phiLength) * sin(thetaStart + v*thetaLength)
            );
            normals.push_back(normalize(vertex));
            uvs.emplace_back(u + uOffset, 1.0f - v);

            verticesRow.push_back(index++);
        }
        grid.push_back(std::move(verticesRow));
    }

    // generate indices
    for (size_t iy = 0; iy < heightSegments; ++iy) {
        for (size_t ix = 0; ix < widthSegments; ++ix) {
            const uint32_t a = grid.at(iy  ).at(ix+1);
            const uint32_t b = grid.at(iy  ).at(ix);
            const uint32_t c = grid.at(iy+1).at(ix);
            const uint32_t d = grid.at(iy+1).at(ix+1);

            if (iy != 0 || thetaStart > 0_deg) {
                indices.insert(indices.end(), {a, b, d});
            }
            if (iy != (heightSegments-1) || thetaEnd < 180_deg) {
                indices.insert(indices.end(), {b, c, d});
            }
        }
    }

    m_Mesh.setVerts(vertices);
    m_Mesh.setNormals(normals);
    m_Mesh.setTexCoords(uvs);
    m_Mesh.setIndices(indices);
}
