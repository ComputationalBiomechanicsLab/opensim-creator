#include "PlaneGeometry.h"

#include <oscar/Graphics/Mesh.h>

#include <cstddef>
#include <cstdint>
#include <vector>

using namespace osc;

osc::PlaneGeometry::PlaneGeometry(
    float width,
    float height,
    size_t widthSegments,
    size_t heightSegments)
{
    float const halfWidth = width/2.0f;
    float const halfHeight = height/2.0f;
    size_t const gridX = widthSegments;
    size_t const gridY = heightSegments;
    size_t const gridX1 = gridX + 1;
    size_t const gridY1 = gridY + 1;
    float const segmentWidth = width / static_cast<float>(gridX);
    float const segmentHeight = height / static_cast<float>(gridY);

    std::vector<uint32_t> indices;
    std::vector<Vec3> vertices;
    std::vector<Vec3> normals;
    std::vector<Vec2> uvs;

    // generate vertices, normals, and uvs
    for (size_t iy = 0; iy < gridY1; ++iy) {
        float const y = static_cast<float>(iy) * segmentHeight - halfHeight;
        for (size_t ix = 0; ix < gridX1; ++ix) {
            float const x = static_cast<float>(ix) * segmentWidth - halfWidth;

            vertices.emplace_back(x, -y, 0.0f);
            normals.emplace_back(0.0f, 0.0f, 1.0f);
            uvs.emplace_back(
                static_cast<float>(ix)/static_cast<float>(gridX),
                1.0f - static_cast<float>(iy)/static_cast<float>(gridY)
            );
        }
    }

    for (size_t iy = 0; iy < gridY; ++iy) {
        for (size_t ix = 0; ix < gridX; ++ix) {
            auto const a = static_cast<uint32_t>((ix + 0) + gridX1*(iy + 0));
            auto const b = static_cast<uint32_t>((ix + 0) + gridX1*(iy + 1));
            auto const c = static_cast<uint32_t>((ix + 1) + gridX1*(iy + 1));
            auto const d = static_cast<uint32_t>((ix + 1) + gridX1*(iy + 0));
            indices.insert(indices.end(), {a, b, d});
            indices.insert(indices.end(), {b, c, d});
        }
    }

    m_Mesh.setVerts(vertices);
    m_Mesh.setNormals(normals);
    m_Mesh.setTexCoords(uvs);
    m_Mesh.setIndices(indices);
}
