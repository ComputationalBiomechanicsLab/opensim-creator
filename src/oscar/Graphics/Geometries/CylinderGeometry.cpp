#include "CylinderGeometry.h"

#include <oscar/Graphics/Mesh.h>
#include <oscar/Graphics/MeshTopology.h>
#include <oscar/Graphics/SubMeshDescriptor.h>
#include <oscar/Maths/Angle.h>
#include <oscar/Maths/TrigonometricFunctions.h>
#include <oscar/Maths/UnitVec3.h>

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

using namespace osc;

osc::CylinderGeometry::CylinderGeometry(
    float radius_top,
    float radius_bottom,
    float height,
    size_t num_radial_segments,
    size_t num_height_segments,
    bool open_ended,
    Radians theta_start,
    Radians theta_length)
{
    // this implementation was initially hand-ported from threejs (CylinderGeometry)

    const auto fnum_radial_segments = static_cast<float>(num_radial_segments);
    const auto fnum_height_segments = static_cast<float>(num_height_segments);

    std::vector<uint32_t> indices;
    std::vector<Vec3> vertices;
    std::vector<Vec3> normals;
    std::vector<Vec2> uvs;

    uint32_t index = 0;
    std::vector<std::vector<uint32_t>> index_array;
    const float half_height = 0.5f * height;
    size_t group_start = 0;
    std::vector<SubMeshDescriptor> groups;

    const auto generate_torso = [&]()
    {
        // used to calculate normal
        const float slope = (radius_bottom - radius_top) / height;

        // generate vertices, normals, and uvs
        size_t group_count = 0;
        for (size_t y = 0; y <= num_height_segments; ++y) {
            std::vector<uint32_t> row_indices;
            const float v = static_cast<float>(y)/fnum_height_segments;
            const float radius = v * (radius_bottom - radius_top) + radius_top;
            for (size_t x = 0; x <= num_radial_segments; ++x) {
                const auto fx = static_cast<float>(x);
                const float u = fx/fnum_radial_segments;
                const Radians theta = u*theta_length + theta_start;
                const float sin_theta = sin(theta);
                const float cos_theta = cos(theta);

                vertices.emplace_back(
                    radius * sin_theta,
                    (-v * height) + half_height,
                    radius * cos_theta
                );
                normals.push_back(UnitVec3{sin_theta, slope, cos_theta});
                uvs.emplace_back(u, 1 - v);
                row_indices.push_back(index++);
            }
            index_array.push_back(std::move(row_indices));
        }

        // generate indices
        for (size_t x = 0; x < num_radial_segments; ++x) {
            for (size_t y = 0; y < num_height_segments; ++y) {
                const auto a = index_array.at(y+0).at(x+0);
                const auto b = index_array.at(y+1).at(x+0);
                const auto c = index_array.at(y+1).at(x+1);
                const auto d = index_array.at(y+0).at(x+1);
                indices.insert(indices.end(), {a, b, d});
                indices.insert(indices.end(), {b, c, d});
                group_count += 6;
            }
        }

        groups.emplace_back(group_start, group_count, MeshTopology::Triangles);
        group_start += group_count;
    };

    const auto generate_cap = [&](bool top)
    {
        size_t group_count = 0;

        const auto radius = top ? radius_top : radius_bottom;
        const auto sign = top ? 1.0f : -1.0f;

        // first, generate the center vertex data of the cap.
        // Because, the geometry needs one set of uvs per face
        // we must generate a center vertex per face/segment

        const auto center_index_start = index;  // save first center vertex
        for (size_t x = 1; x <= num_radial_segments; ++x) {
            vertices.emplace_back(0.0f, sign*half_height, 0.0f);
            normals.emplace_back(0.0f, sign, 0.0f);
            uvs.emplace_back(0.5f, 0.5f);
            ++index;
        }
        const auto center_index_end = index;  // save last center vertex

        // generate surrounding vertices, normals, and uvs
        for (size_t x = 0; x <= num_radial_segments; ++x) {
            const auto fx = static_cast<float>(x);
            const float u = fx / fnum_radial_segments;
            const Radians theta = u * theta_length + theta_start;
            const float cos_theta = cos(theta);
            const float sin_theta = sin(theta);

            vertices.emplace_back(radius * sin_theta, half_height * sign, radius * cos_theta);
            normals.emplace_back(0.0f, sign, 0.0f);
            uvs.emplace_back(
                (cos_theta * 0.5f) + 0.5f,
                (sin_theta * 0.5f * sign) + 0.5f
            );
            ++index;
        }

        // generate indices
        for (size_t x = 0; x < num_radial_segments; ++x) {
            const auto c = static_cast<uint32_t>(center_index_start + x);
            const auto i = static_cast<uint32_t>(center_index_end + x);

            if (top) {
                indices.insert(indices.end(), {i, i+1, c});
            }
            else {
                indices.insert(indices.end(), {i+1, i, c});
            }
            group_count += 3;
        }

        groups.emplace_back(group_start, group_count, MeshTopology::Triangles);
        group_start += group_count;
    };

    generate_torso();
    if (not open_ended) {
        if (radius_top > 0.0f) {
            generate_cap(true);
        }
        if (radius_bottom > 0.0f) {
            generate_cap(false);
        }
    }

    mesh_.set_vertices(vertices);
    mesh_.set_normals(normals);
    mesh_.set_tex_coords(uvs);
    mesh_.set_indices(indices);
    mesh_.set_submesh_descriptors(groups);
}
