#include "LatheGeometry.h"

#include <oscar/Graphics/Mesh.h>
#include <oscar/Maths/Angle.h>
#include <oscar/Maths/CommonFunctions.h>
#include <oscar/Maths/GeometricFunctions.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/Maths/Vec3.h>

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

using namespace osc;
using namespace osc::literals;

osc::LatheGeometry::LatheGeometry(
    std::span<const Vec2> points,
    size_t num_segments,
    Radians phi_start,
    Radians phi_length)
{
    // the implementation of this was initially translated from `three.js`'s
    // `LatheGeometry`, which has excellent documentation and source code. The
    // code was then subsequently mutated to suit OSC, C++ etc.
    //
    // https://threejs.org/docs/#api/en/geometries/LatheGeometry

    if (points.size() <= 2) {
        return;  // edge-case: requires at least 2 points
    }

    phi_length = clamp(phi_length, 0_deg, 360_deg);

    std::vector<uint32_t> indices;
    std::vector<Vec3> vertices;
    std::vector<Vec2> uvs;
    std::vector<Vec3> init_normals;
    std::vector<Vec3> normals;

    const auto fnum_segments = static_cast<float>(num_segments);
    const auto recip_num_segments = 1.0f/fnum_segments;
    Vec3 previous_normal{};

    // pre-compute normals for initial "meridian"
    {
        // first vertex
        const Vec2 dv = points[1] - points[0];
        const Vec3 normal = {dv.y * 1.0f, -dv.x, dv.y * 0.0f};

        init_normals.push_back(normalize(normal));
        previous_normal = normal;
    }
    // in-between vertices
    for (size_t i = 1; i < points.size()-1; ++i) {
        const Vec2 dv = points[i+1] - points[i];
        const Vec3 normal = {dv.y * 1.0f, -dv.x, dv.y * 0.0f};

        init_normals.push_back(normalize(normal + previous_normal));
        previous_normal = normal;
    }
    // last vertex
    init_normals.push_back(previous_normal);

    // generate vertices, uvs, and normals
    for (size_t i = 0; i <= num_segments; ++i) {
        const auto fi = static_cast<float>(i);
        const auto phi = phi_start + fi*recip_num_segments*phi_length;
        const auto sin_phi = sin(phi);
        const auto cos_phi = cos(phi);

        for (size_t j = 0; j <= points.size()-1; ++j) {
            const auto fj = static_cast<float>(j);

            vertices.emplace_back(
                points[j].x * sin_phi,
                points[j].y,
                points[j].x * cos_phi
            );
            uvs.emplace_back(
                fi / fnum_segments,
                fj / static_cast<float>(points.size()-1)
            );
            normals.emplace_back(
                init_normals[j].x * sin_phi,
                init_normals[j].y,
                init_normals[j].x * cos_phi
            );
        }
    }

    // indices
    for (size_t i = 0; i < num_segments; ++i) {
        for (size_t j = 0; j < points.size()-1; ++j) {
            const size_t base = j + i*points.size();

            const auto a = static_cast<uint32_t>(base);
            const auto b = static_cast<uint32_t>(base + points.size());
            const auto c = static_cast<uint32_t>(base + points.size() + 1);
            const auto d = static_cast<uint32_t>(base + 1);

            indices.insert(indices.end(), {a, b, d});
            indices.insert(indices.end(), {c, d, b});
        }
    }

    mesh_.set_vertices(vertices);
    mesh_.set_normals(normals);
    mesh_.set_tex_coords(uvs);
    mesh_.set_indices(indices);
}
