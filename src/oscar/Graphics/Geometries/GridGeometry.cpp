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
    size_t num_divisions)
{
    constexpr float z = 0.0f;
    const float min = -0.5f*size;
    const float max =  0.5f*size;

    const float step_size = (max - min) / static_cast<float>(num_divisions);

    const size_t num_lines = num_divisions + 1;

    std::vector<Vec3> vertices;
    vertices.reserve(4 * num_lines);
    std::vector<uint32_t> indices;
    indices.reserve(4 * num_lines);
    std::vector<Vec3> normals;
    normals.reserve(4 * num_lines);
    uint32_t index = 0;

    auto push = [&index, &vertices, &indices, &normals](const Vec3& pos)
    {
        vertices.push_back(pos);
        indices.push_back(index++);
        normals.emplace_back(0.0f, 0.0f, 1.0f);
    };

    // lines parallel to X axis
    for (size_t i = 0; i < num_lines; ++i) {
        const float y = min + static_cast<float>(i) * step_size;

        push({-1.0f, y, z});
        push({+1.0f, y, z});
    }

    // lines parallel to Y axis
    for (size_t i = 0; i < num_lines; ++i) {
        const float x = min + static_cast<float>(i) * step_size;

        push({x, -1.0f, z});
        push({x, +1.0f, z});
    }

    mesh_.setTopology(MeshTopology::Lines);
    mesh_.setVerts(vertices);
    mesh_.setNormals(normals);
    mesh_.setIndices(indices);
}
