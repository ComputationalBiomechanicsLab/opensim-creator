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

Mesh osc::LatheGeometry::generate_mesh(
    std::span<Vec2 const> points,
    size_t segments,
    Radians phiStart,
    Radians phiLength)
{
    // this implementation was initially hand-ported from threejs (LatheGeometry)

    if (points.size() <= 2) {
        return Mesh{};  // edge-case: requires at least 2 points
    }

    phiLength = clamp(phiLength, 0_deg, 360_deg);

    std::vector<uint32_t> indices;
    std::vector<Vec3> vertices;
    std::vector<Vec2> uvs;
    std::vector<Vec3> initNormals;
    std::vector<Vec3> normals;

    auto const fsegments = static_cast<float>(segments);
    auto const inverseSegments = 1.0f/fsegments;
    Vec3 prevNormal{};

    // pre-compute normals for initial "meridian"
    {
        // first vertex
        Vec2 const dv = points[1] - points[0];
        Vec3 normal = {dv.y * 1.0f, -dv.x, dv.y * 0.0f};

        initNormals.push_back(normalize(normal));
        prevNormal = normal;
    }
    // in-between vertices
    for (size_t i = 1; i < points.size()-1; ++i) {
        Vec2 const dv = points[i+1] - points[i];
        Vec3 normal = {dv.y * 1.0f, -dv.x, dv.y * 0.0f};

        initNormals.push_back(normalize(normal + prevNormal));
        prevNormal = normal;
    }
    // last vertex
    initNormals.push_back(prevNormal);

    // generate vertices, uvs, and normals
    for (size_t i = 0; i <= segments; ++i) {
        auto const fi = static_cast<float>(i);
        auto const phi = phiStart + fi*inverseSegments*phiLength;
        auto const sinPhi = sin(phi);
        auto const cosPhi = cos(phi);

        for (size_t j = 0; j <= points.size()-1; ++j) {
            auto const fj = static_cast<float>(j);

            vertices.emplace_back(
                points[j].x * sinPhi,
                points[j].y,
                points[j].x * cosPhi
            );
            uvs.emplace_back(
                fi / fsegments,
                fj / static_cast<float>(points.size()-1)
            );
            normals.emplace_back(
                initNormals[j].x * sinPhi,
                initNormals[j].y,
                initNormals[j].x * cosPhi
            );
        }
    }

    // indices
    for (size_t i = 0; i < segments; ++i) {
        for (size_t j = 0; j < points.size()-1; ++j) {
            size_t const base = j + i*points.size();

            auto const a = static_cast<uint32_t>(base);
            auto const b = static_cast<uint32_t>(base + points.size());
            auto const c = static_cast<uint32_t>(base + points.size() + 1);
            auto const d = static_cast<uint32_t>(base + 1);

            indices.insert(indices.end(), {a, b, d});
            indices.insert(indices.end(), {c, d, b});
        }
    }

    Mesh rv;
    rv.setVerts(vertices);
    rv.setNormals(normals);
    rv.setTexCoords(uvs);
    rv.setIndices(indices);
    return rv;
}
