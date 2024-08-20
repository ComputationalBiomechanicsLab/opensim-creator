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

osc::SphereGeometry::SphereGeometry(const Params& p)
{
    // the implementation of this was initially translated from `three.js`'s
    // `SphereGeometry`, which has excellent documentation and source code. The
    // code was then subsequently mutated to suit OSC, C++ etc.
    //
    // https://threejs.org/docs/#api/en/geometries/SphereGeometry

    const auto num_width_segments = max(3_uz, p.num_width_segments);
    const auto num_height_segments = max(2_uz, p.num_height_segments);
    const auto fnum_width_segments = static_cast<float>(num_width_segments);
    const auto fnum_height_segments = static_cast<float>(num_height_segments);
    const auto theta_end = min(p.theta_start + p.theta_length, 180_deg);

    uint32_t index = 0;
    std::vector<std::vector<uint32_t>> grid;

    std::vector<uint32_t> indices;
    std::vector<Vec3> vertices;
    std::vector<Vec3> normals;
    std::vector<Vec2> uvs;

    // generate vertices, normals, and uvs
    for (size_t iy = 0; iy <= num_height_segments; ++iy) {
        std::vector<uint32_t> row_vertices;
        const float v = static_cast<float>(iy) / fnum_height_segments;

        // edge-case: poles
        float u_offset = 0.0f;
        if (iy == 0 and p.theta_start == 0_deg) {
            u_offset = 0.5f / fnum_width_segments;
        }
        else if (iy == num_height_segments and theta_end == 180_deg) {
            u_offset = -0.5f / fnum_width_segments;
        }

        for (size_t ix = 0; ix <= num_width_segments; ++ix) {
            const float u = static_cast<float>(ix) / fnum_width_segments;

            const Vec3& vertex = vertices.emplace_back(
                -p.radius * cos(p.phi_start   + u*p.phi_length)    * sin(p.theta_start + v*p.theta_length),
                 p.radius * cos(p.theta_start + v*p.theta_length),
                 p.radius * sin(p.phi_start   + u*p.phi_length)    * sin(p.theta_start + v*p.theta_length)
            );
            normals.push_back(normalize(vertex));
            uvs.emplace_back(u + u_offset, 1.0f - v);

            row_vertices.push_back(index++);
        }
        grid.push_back(std::move(row_vertices));
    }

    // generate indices
    for (size_t iy = 0; iy < num_height_segments; ++iy) {
        for (size_t ix = 0; ix < num_width_segments; ++ix) {
            const uint32_t a = grid.at(iy  ).at(ix+1);
            const uint32_t b = grid.at(iy  ).at(ix);
            const uint32_t c = grid.at(iy+1).at(ix);
            const uint32_t d = grid.at(iy+1).at(ix+1);

            if (iy != 0 or p.theta_start > 0_deg) {
                indices.insert(indices.end(), {a, b, d});
            }
            if (iy != (num_height_segments-1) or theta_end < 180_deg) {
                indices.insert(indices.end(), {b, c, d});
            }
        }
    }

    set_vertices(vertices);
    set_normals(normals);
    set_tex_coords(uvs);
    set_indices(indices);
}
