#include "BoxGeometry.h"

#include <oscar/Graphics/Mesh.h>
#include <oscar/Graphics/MeshTopology.h>
#include <oscar/Graphics/SubMeshDescriptor.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/Maths/Vec3.h>

#include <cstddef>
#include <cstdint>
#include <vector>

using namespace osc;

osc::BoxGeometry::BoxGeometry(
    float width,
    float height,
    float depth,
    size_t num_width_segments,
    size_t num_height_segments,
    size_t num_depth_segments)
{
    // the implementation/API of this was initially translated from `three.js`'s
    // `BoxGeometry`, which has excellent documentation and source code. The
    // code was then subsequently mutated to suit OSC, C++ etc.
    //
    // https://threejs.org/docs/#api/en/geometries/BoxGeometry

    std::vector<uint32_t> indices;
    std::vector<Vec3> vertices;
    std::vector<Vec3> normals;
    std::vector<Vec2> uvs;
    std::vector<SubMeshDescriptor> submeshes;  // for multi-material support

    // helper variables
    size_t num_vertices = 0;
    size_t group_start = 0;

    // helper function
    const auto build_plane = [&indices, &vertices, &normals, &uvs, &submeshes, &num_vertices, &group_start](
        Vec3::size_type u,
        Vec3::size_type v,
        Vec3::size_type w,
        float udir,
        float vdir,
        Vec3 dims,
        size_t grid_x,
        size_t grid_y)
    {
        const float segment_width = dims.x / static_cast<float>(grid_x);
        const float segment_height = dims.y / static_cast<float>(grid_y);

        const float half_width = 0.5f * dims.x;
        const float half_height = 0.5f * dims.y;
        const float half_depth = 0.5f * dims.z;

        const size_t grid_x1 = grid_x + 1;
        const size_t grid_y1 = grid_y + 1;

        size_t vertex_count = 0;
        size_t group_count = 0;

        // generate vertices, normals, and UVs
        for (size_t iy = 0; iy < grid_y1; ++iy) {
            const float y = static_cast<float>(iy)*segment_height - half_height;
            for (size_t ix = 0; ix < grid_x1; ++ix) {
                const float x = static_cast<float>(ix)*segment_width - half_width;

                Vec3 vertex{};
                vertex[u] = x*udir;
                vertex[v] = y*vdir;
                vertex[w] = half_depth;
                vertices.push_back(vertex);

                Vec3 normal{};
                normal[u] = 0.0f;
                normal[v] = 0.0f;
                normal[w] = dims.z > 0.0f ? 1.0f : -1.0f;
                normals.push_back(normal);

                uvs.emplace_back(static_cast<float>(ix)/static_cast<float>(grid_x), 1.0f - static_cast<float>(iy)/static_cast<float>(grid_y));

                ++vertex_count;
            }
        }

        // indices (two triangles, or 6 indices, per segment)
        for (size_t iy = 0; iy < grid_y; ++iy) {
            for (size_t ix = 0; ix < grid_x; ++ix) {
                const auto a = static_cast<uint32_t>(num_vertices +  ix      + (grid_x1 *  iy     ));
                const auto b = static_cast<uint32_t>(num_vertices +  ix      + (grid_x1 * (iy + 1)));
                const auto c = static_cast<uint32_t>(num_vertices + (ix + 1) + (grid_x1 * (iy + 1)));
                const auto d = static_cast<uint32_t>(num_vertices + (ix + 1) + (grid_x1 *  iy     ));

                indices.insert(indices.end(), {a, b, d});
                indices.insert(indices.end(), {b, c, d});

                group_count += 6;
            }
        }

        // add submesh description
        submeshes.emplace_back(group_start, group_count, MeshTopology::Triangles);
        group_start += group_count;
        num_vertices += vertex_count;
    };

    // build each side of the box
    build_plane(2, 1, 0, -1.0f, -1.0f, {depth, height,  width},  num_depth_segments, num_height_segments);  // px
    build_plane(2, 1, 0,  1.0f, -1.0f, {depth, height, -width},  num_depth_segments, num_height_segments);  // nx
    build_plane(0, 2, 1,  1.0f,  1.0f, {width, depth,   height}, num_width_segments, num_depth_segments);   // py
    build_plane(0, 2, 1,  1.0f, -1.0f, {width, depth,  -height}, num_width_segments, num_depth_segments);   // ny
    build_plane(0, 1, 2,  1.0f, -1.0f, {width, height,  depth},  num_width_segments, num_height_segments);  // pz
    build_plane(0, 1, 2, -1.0f, -1.0f, {width, height, -depth},  num_width_segments, num_height_segments);  // nz

    // the first submesh is "the entire cube"
    submeshes.insert(submeshes.begin(), SubMeshDescriptor{0, group_start, MeshTopology::Triangles});

    // build geometry
    mesh_.setVerts(vertices);
    mesh_.setNormals(normals);
    mesh_.setTexCoords(uvs);
    mesh_.setIndices(indices);
    mesh_.setSubmeshDescriptors(submeshes);
}
