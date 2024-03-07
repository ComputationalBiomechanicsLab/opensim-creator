#include "GridGeometry.h"

#include <oscar/Graphics/Mesh.h>
#include <oscar/Graphics/MeshTopology.h>
#include <oscar/Maths/Vec3.h>

#include <cstddef>
#include <cstdint>
#include <vector>

using namespace osc;

osc::GridGeometry::GridGeometry(
    float size,
    size_t divisions)
{
    constexpr float z = 0.0f;
    float const min = -size/2.0f;
    float const max =  size/2.0f;

    float const stepSize = (max - min) / static_cast<float>(divisions);

    size_t const nlines = divisions + 1;

    std::vector<Vec3> vertices;
    vertices.reserve(4 * nlines);
    std::vector<uint32_t> indices;
    indices.reserve(4 * nlines);
    std::vector<Vec3> normals;
    normals.reserve(4 * nlines);
    uint32_t index = 0;

    auto push = [&index, &vertices, &indices, &normals](Vec3 const& pos)
    {
        vertices.push_back(pos);
        indices.push_back(index++);
        normals.emplace_back(0.0f, 0.0f, 1.0f);
    };

    // lines parallel to X axis
    for (size_t i = 0; i < nlines; ++i) {
        float const y = min + static_cast<float>(i) * stepSize;

        push({-1.0f, y, z});
        push({+1.0f, y, z});
    }

    // lines parallel to Y axis
    for (size_t i = 0; i < nlines; ++i) {
        float const x = min + static_cast<float>(i) * stepSize;

        push({x, -1.0f, z});
        push({x, +1.0f, z});
    }

    m_Mesh.setTopology(MeshTopology::Lines);
    m_Mesh.setVerts(vertices);
    m_Mesh.setNormals(normals);
    m_Mesh.setIndices(indices);
}
