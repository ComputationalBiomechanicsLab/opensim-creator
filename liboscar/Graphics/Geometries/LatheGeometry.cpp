#include "LatheGeometry.h"

#include <liboscar/Graphics/Mesh.h>
#include <liboscar/Maths/Angle.h>
#include <liboscar/Maths/CommonFunctions.h>
#include <liboscar/Maths/GeometricFunctions.h>
#include <liboscar/Maths/Vector2.h>
#include <liboscar/Maths/Vector3.h>

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

using namespace osc;
using namespace osc::literals;

osc::LatheGeometry::LatheGeometry(const Params& p)
{
    // the implementation of this was initially translated from `three.js`'s
    // `LatheGeometry`, which has excellent documentation and source code. The
    // code was then subsequently mutated to suit OSC, C++ etc.
    //
    // https://threejs.org/docs/#api/en/geometries/LatheGeometry

    if (p.points.size() <= 2) {
        return;  // edge-case: requires at least 2 points
    }

    const auto phi_length = clamp(p.phi_length, 0_deg, 360_deg);

    std::vector<uint32_t> indices;
    std::vector<Vector3> vertices;
    std::vector<Vector2> uvs;
    std::vector<Vector3> init_normals;
    std::vector<Vector3> normals;

    const auto fnum_segments = static_cast<float>(p.num_segments);
    const auto recip_num_segments = 1.0f/fnum_segments;
    Vector3 previous_normal{};

    // pre-compute normals for initial "meridian"
    {
        // first vertex
        const Vector2 dv = p.points[1] - p.points[0];
        const Vector3 normal = {dv.y * 1.0f, -dv.x, dv.y * 0.0f};

        init_normals.push_back(normalize(normal));
        previous_normal = normal;
    }
    // in-between vertices
    for (size_t i = 1; i < p.points.size()-1; ++i) {
        const Vector2 dv = p.points[i+1] - p.points[i];
        const Vector3 normal = {dv.y * 1.0f, -dv.x, dv.y * 0.0f};

        init_normals.push_back(normalize(normal + previous_normal));
        previous_normal = normal;
    }
    // last vertex
    init_normals.push_back(previous_normal);

    // generate vertices, uvs, and normals
    for (size_t i = 0; i <= p.num_segments; ++i) {
        const auto fi = static_cast<float>(i);
        const auto phi = p.phi_start + fi*recip_num_segments*phi_length;
        const auto sin_phi = sin(phi);
        const auto cos_phi = cos(phi);

        for (size_t j = 0; j <= p.points.size()-1; ++j) {
            const auto fj = static_cast<float>(j);

            vertices.emplace_back(
                p.points[j].x * sin_phi,
                p.points[j].y,
                p.points[j].x * cos_phi
            );
            uvs.emplace_back(
                fi / fnum_segments,
                fj / static_cast<float>(p.points.size()-1)
            );
            normals.emplace_back(
                init_normals[j].x * sin_phi,
                init_normals[j].y,
                init_normals[j].x * cos_phi
            );
        }
    }

    // indices
    for (size_t i = 0; i < p.num_segments; ++i) {
        for (size_t j = 0; j < p.points.size()-1; ++j) {
            const size_t base = j + i*p.points.size();

            const auto a = static_cast<uint32_t>(base);
            const auto b = static_cast<uint32_t>(base + p.points.size());
            const auto c = static_cast<uint32_t>(base + p.points.size() + 1);
            const auto d = static_cast<uint32_t>(base + 1);

            indices.insert(indices.end(), {a, b, d});
            indices.insert(indices.end(), {c, d, b});
        }
    }

    set_vertices(vertices);
    set_normals(normals);
    set_tex_coords(uvs);
    set_indices(indices);
}
