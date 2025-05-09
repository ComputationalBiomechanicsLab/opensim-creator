#include "GridGeometry.h"

#include <liboscar/Graphics/Mesh.h>
#include <liboscar/Graphics/MeshTopology.h>
#include <liboscar/Maths/Vec3.h>

#include <cstddef>
#include <cstdint>
#include <vector>

using namespace osc;

osc::GridGeometry::GridGeometry(const Params& p)
{
    const float min = -0.5f*p.size;
    const float max =  0.5f*p.size;

    const float step_size = (max - min) / static_cast<float>(p.num_divisions);

    const size_t num_lines = p.num_divisions + 1;

    std::vector<Vec3> vertices;
    vertices.reserve(4 * num_lines);
    std::vector<uint32_t> indices;
    indices.reserve(4 * num_lines);
    std::vector<Vec3> normals;
    normals.reserve(4 * num_lines);
    uint32_t index = 0;

    auto push = [&index, &vertices, &indices, &normals](const Vec3& vertex)
    {
        vertices.push_back(vertex);
        indices.push_back(index++);
        normals.emplace_back(0.0f, 0.0f, 1.0f);
    };

    // lines parallel to X axis
    for (size_t i = 0; i < num_lines; ++i) {
        const float y = min + static_cast<float>(i) * step_size;

        push({min, y, 0.0f});
        push({max, y, 0.0f});
    }

    // lines parallel to Y axis
    for (size_t i = 0; i < num_lines; ++i) {
        const float x = min + static_cast<float>(i) * step_size;

        push({x, min, 0.0f});
        push({x, max, 0.0f});
    }

    set_topology(MeshTopology::Lines);
    set_vertices(vertices);
    set_normals(normals);
    set_indices(indices);
}
