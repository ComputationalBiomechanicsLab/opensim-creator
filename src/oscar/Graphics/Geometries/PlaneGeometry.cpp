#include "PlaneGeometry.h"

#include <oscar/Graphics/Mesh.h>

#include <cstddef>
#include <cstdint>
#include <vector>

using namespace osc;

osc::PlaneGeometry::PlaneGeometry(
    float width,
    float height,
    size_t num_width_segments,
    size_t num_height_segments)
{
    const float half_width = 0.5f * width;
    const float half_height = 0.5f * height;
    const size_t grid_x = num_width_segments;
    const size_t grid_y = num_height_segments;
    const size_t grid_x1 = grid_x + 1;
    const size_t grid_y1 = grid_y + 1;
    const float segment_width = width / static_cast<float>(grid_x);
    const float segment_height = height / static_cast<float>(grid_y);

    std::vector<uint32_t> indices;
    std::vector<Vec3> vertices;
    std::vector<Vec3> normals;
    std::vector<Vec2> uvs;

    // generate vertices, normals, and uvs
    for (size_t iy = 0; iy < grid_y1; ++iy) {
        const float y = static_cast<float>(iy) * segment_height - half_height;
        for (size_t ix = 0; ix < grid_x1; ++ix) {
            const float x = static_cast<float>(ix) * segment_width - half_width;

            vertices.emplace_back(x, -y, 0.0f);
            normals.emplace_back(0.0f, 0.0f, 1.0f);
            uvs.emplace_back(
                static_cast<float>(ix)/static_cast<float>(grid_x),
                1.0f - static_cast<float>(iy)/static_cast<float>(grid_y)
            );
        }
    }

    for (size_t iy = 0; iy < grid_y; ++iy) {
        for (size_t ix = 0; ix < grid_x; ++ix) {
            const auto a = static_cast<uint32_t>((ix + 0) + grid_x1*(iy + 0));
            const auto b = static_cast<uint32_t>((ix + 0) + grid_x1*(iy + 1));
            const auto c = static_cast<uint32_t>((ix + 1) + grid_x1*(iy + 1));
            const auto d = static_cast<uint32_t>((ix + 1) + grid_x1*(iy + 0));
            indices.insert(indices.end(), {a, b, d});
            indices.insert(indices.end(), {b, c, d});
        }
    }

    mesh_.setVerts(vertices);
    mesh_.setNormals(normals);
    mesh_.setTexCoords(uvs);
    mesh_.setIndices(indices);
}
