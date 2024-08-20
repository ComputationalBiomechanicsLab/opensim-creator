#include "RingGeometry.h"

#include <oscar/Graphics/Mesh.h>
#include <oscar/Shims/Cpp23/cstddef.h>
#include <oscar/Maths/Angle.h>
#include <oscar/Maths/CommonFunctions.h>
#include <oscar/Maths/TrigonometricFunctions.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/Maths/Vec3.h>

#include <cstddef>
#include <cstdint>
#include <vector>

using namespace osc;
using namespace osc::literals;

osc::RingGeometry::RingGeometry(const Params& p)
{
    // the implementation of this was initially translated from `three.js`'s
    // `RingGeometry`, which has excellent documentation and source code. The
    // code was then subsequently mutated to suit OSC, C++ etc.
    //
    // https://threejs.org/docs/#api/en/geometries/RingGeometry

    const auto num_theta_segments = max(3_uz, p.num_theta_segments);
    const auto num_phi_segments = max(1_uz, p.num_phi_segments);
    const auto fnum_theta_segments = static_cast<float>(num_theta_segments);
    const auto fnum_phi_segments = static_cast<float>(num_phi_segments);

    std::vector<uint32_t> indices;
    std::vector<Vec3> vertices;
    std::vector<Vec3> normals;
    std::vector<Vec2> uvs;

    float radius = p.inner_radius;
    float radius_step = (p.outer_radius - p.inner_radius)/fnum_phi_segments;

    // generate vertices, normals, and uvs
    for (size_t j = 0; j <= num_phi_segments; ++j) {
        for (size_t i = 0; i <= num_theta_segments; ++i) {
            const auto fi = static_cast<float>(i);
            const Radians segment = p.theta_start + (fi/fnum_theta_segments * p.theta_length);

            const Vec3& v = vertices.emplace_back(radius * cos(segment), radius * sin(segment), 0.0f);
            normals.emplace_back(0.0f, 0.0f, 1.0f);
            uvs.emplace_back(
                (v.x/p.outer_radius + 1.0f) / 2.0f,
                (v.y/p.outer_radius + 1.0f) / 2.0f
            );
        }
        radius += radius_step;
    }

    for (size_t j = 0; j < num_phi_segments; ++j) {
        const size_t theta_segment_level = j * (num_theta_segments + 1);
        for (size_t i = 0; i < num_theta_segments; ++i) {
            const size_t segment = i + theta_segment_level;

            const auto a = static_cast<uint32_t>(segment);
            const auto b = static_cast<uint32_t>(segment + num_theta_segments + 1);
            const auto c = static_cast<uint32_t>(segment + num_theta_segments + 2);
            const auto d = static_cast<uint32_t>(segment + 1);

            indices.insert(indices.end(), {a, b, d});
            indices.insert(indices.end(), {b, c, d});
        }
    }

    set_vertices(vertices);
    set_normals(normals);
    set_tex_coords(uvs);
    set_indices(indices);
}
