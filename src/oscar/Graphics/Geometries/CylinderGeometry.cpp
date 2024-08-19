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

osc::CylinderGeometry::CylinderGeometry(const Params& p)
{
    // the implementation of this was initially translated from `three.js`'s
    // `CylinderGeometry`, which has excellent documentation and source code. The
    // code was then subsequently mutated to suit OSC, C++ etc.
    //
    // https://threejs.org/docs/#api/en/geometries/CylinderGeometry

    const auto fnum_radial_segments = static_cast<float>(p.num_radial_segments);
    const auto fnum_height_segments = static_cast<float>(p.num_height_segments);

    std::vector<uint32_t> indices;
    std::vector<Vec3> vertices;
    std::vector<Vec3> normals;
    std::vector<Vec2> uvs;

    uint32_t index = 0;
    std::vector<std::vector<uint32_t>> index_array;
    const float half_height = 0.5f * p.height;
    size_t group_start = 0;
    std::vector<SubMeshDescriptor> groups;

    const auto generate_torso = [&]()
    {
        // used to calculate normal
        const float slope = (p.radius_bottom - p.radius_top) / p.height;

        // generate vertices, normals, and uvs
        size_t group_count = 0;
        for (size_t y = 0; y <= p.num_height_segments; ++y) {
            std::vector<uint32_t> row_indices;
            const float v = static_cast<float>(y)/fnum_height_segments;
            const float radius = v * (p.radius_bottom - p.radius_top) + p.radius_top;
            for (size_t x = 0; x <= p.num_radial_segments; ++x) {
                const auto fx = static_cast<float>(x);
                const float u = fx/fnum_radial_segments;
                const Radians theta = u*p.theta_length + p.theta_start;
                const float sin_theta = sin(theta);
                const float cos_theta = cos(theta);

                vertices.emplace_back(
                    radius * sin_theta,
                    (-v * p.height) + half_height,
                    radius * cos_theta
                );
                normals.push_back(UnitVec3{sin_theta, slope, cos_theta});
                uvs.emplace_back(u, 1 - v);
                row_indices.push_back(index++);
            }
            index_array.push_back(std::move(row_indices));
        }

        // generate indices
        for (size_t x = 0; x < p.num_radial_segments; ++x) {
            for (size_t y = 0; y < p.num_height_segments; ++y) {
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

        const auto radius = top ? p.radius_top : p.radius_bottom;
        const auto sign = top ? 1.0f : -1.0f;

        // first, generate the center vertex data of the cap.
        // Because, the geometry needs one set of uvs per face
        // we must generate a center vertex per face/segment

        const auto center_index_start = index;  // save first center vertex
        for (size_t x = 1; x <= p.num_radial_segments; ++x) {
            vertices.emplace_back(0.0f, sign*half_height, 0.0f);
            normals.emplace_back(0.0f, sign, 0.0f);
            uvs.emplace_back(0.5f, 0.5f);
            ++index;
        }
        const auto center_index_end = index;  // save last center vertex

        // generate surrounding vertices, normals, and uvs
        for (size_t x = 0; x <= p.num_radial_segments; ++x) {
            const auto fx = static_cast<float>(x);
            const float u = fx / fnum_radial_segments;
            const Radians theta = u * p.theta_length + p.theta_start;
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
        for (size_t x = 0; x < p.num_radial_segments; ++x) {
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
    if (not p.open_ended) {
        if (p.radius_top > 0.0f) {
            generate_cap(true);
        }
        if (p.radius_bottom > 0.0f) {
            generate_cap(false);
        }
    }

    set_vertices(vertices);
    set_normals(normals);
    set_tex_coords(uvs);
    set_indices(indices);
    set_submesh_descriptors(groups);
}
