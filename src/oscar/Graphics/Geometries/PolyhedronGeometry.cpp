#include "PolyhedronGeometry.h"

#include <oscar/Graphics/Mesh.h>
#include <oscar/Maths/Angle.h>
#include <oscar/Maths/CommonFunctions.h>
#include <oscar/Maths/TrigonometricFunctions.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/Utils/Algorithms.h>
#include <oscar/Utils/Assertions.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

using namespace osc;
using namespace osc::literals;

osc::PolyhedronGeometry::PolyhedronGeometry(
    std::span<const Vec3> vertices,
    std::span<const uint32_t> indices,
    float radius,
    size_t detail_level)
{
    std::vector<Vec3> generated_vertices;
    std::vector<Vec2> uvs;

    const auto subdivide_face = [&generated_vertices](Vec3 a, Vec3 b, Vec3 c, size_t detail)
    {
        const auto num_cols = detail + 1;
        const auto fnum_cols = static_cast<float>(num_cols);

        // we use this multidimensional array as a data structure for creating the subdivision
        std::vector<std::vector<Vec3>> v;
        v.reserve(num_cols+1);

        for (size_t i = 0; i <= num_cols; ++i) {
            const auto fi = static_cast<float>(i);
            const Vec3 aj = lerp(a, c, fi/fnum_cols);
            const Vec3 bj = lerp(b, c, fi/fnum_cols);

            const auto num_rows = num_cols - i;
            const auto fnum_rows = static_cast<float>(num_rows);

            v.emplace_back().reserve(num_rows+1);
            for (size_t j = 0; j <= num_rows; ++j) {
                v.at(i).emplace_back();

                const auto fj = static_cast<float>(j);
                if (j == 0 and i == num_cols) {
                    v.at(i).at(j) = aj;
                }
                else {
                    v.at(i).at(j) = lerp(aj, bj, fj/fnum_rows);
                }
            }
        }

        // construct all of the faces
        for (size_t i = 0; i < num_cols; ++i) {
            for (size_t j = 0; j < 2*(num_cols-i) - 1; ++j) {
                const size_t k = j/2;

                if (j % 2 == 0) {
                    generated_vertices.insert(generated_vertices.end(), {
                        v.at(i).at(k+1),
                        v.at(i+1).at(k),
                        v.at(i).at(k),
                    });
                }
                else {
                    generated_vertices.insert(generated_vertices.end(), {
                        v.at(i).at(k+1),
                        v.at(i+1).at(k+1),
                        v.at(i+1).at(k),
                    });
                }
            }
        }
    };

    const auto subdivide = [&subdivide_face, &vertices, &indices](size_t detail)
    {
        // subdivide each input triangle by the given detail_level
        for (size_t i = 0; i < 3*(indices.size()/3); i += 3) {
            const Vec3 a = at(vertices, at(indices, i+0));
            const Vec3 b = at(vertices, at(indices, i+1));
            const Vec3 c = at(vertices, at(indices, i+2));
            subdivide_face(a, b, c, detail);
        }
    };

    const auto apply_radius = [&generated_vertices](float radius)
    {
        for (Vec3& v : generated_vertices) {
            v = radius * normalize(v);
        }
    };

    // return the angle around the Y axis, CCW when looking from above
    const auto azimuth = [](const Vec3& v) -> Radians
    {
        return atan2(v.z, -v.x);
    };

    const auto correct_uv = [](Vec2& uv, const Vec3& vector, Radians azimuth)
    {
        if ((azimuth < 0_rad) and (uv.x == 1.0f)) {
            uv.x -= 1.0f;
        }
        if ((vector.x == 0.0f) and (vector.z == 0.0f)) {
            uv.x = Turns{azimuth + 0.5_turn}.count();
        }
    };

    const auto correct_uvs = [&generated_vertices, &uvs, &azimuth, &correct_uv]()
    {
        OSC_ASSERT(generated_vertices.size() == uvs.size());
        OSC_ASSERT(generated_vertices.size() % 3 == 0);

        for (size_t i = 0; i < 3*(generated_vertices.size()/3); i += 3) {
            const Vec3 a = generated_vertices[i+0];
            const Vec3 b = generated_vertices[i+1];
            const Vec3 c = generated_vertices[i+2];

            const auto azi = azimuth(centroid_of({a, b, c}));

            correct_uv(uvs[i+0], a, azi);
            correct_uv(uvs[i+1], b, azi);
            correct_uv(uvs[i+2], c, azi);
        }
    };

    const auto correct_seam = [&uvs]()
    {
        // handle case when face straddles the seam, see mrdoob/three.js#3269
        OSC_ASSERT(uvs.size() % 3 == 0);
        for (size_t i = 0; i < 3*(uvs.size()/3); i += 3) {
            const float x0 = uvs[i+0].x;
            const float x1 = uvs[i+1].x;
            const float x2 = uvs[i+2].x;
            const auto [min, max] = minmax({x0, x1, x2});

            // these magic numbers are arbitrary (copied from three.js)
            if (max > 0.9f and min < 0.1f) {
                if (x0 < 0.2f) { uvs[i+0] += 1.0f; }
                if (x1 < 0.2f) { uvs[i+1] += 1.0f; }
                if (x2 < 0.2f) { uvs[i+2] += 1.0f; }
            }
        }
    };

    const auto generate_uvs = [&generated_vertices, &uvs, &azimuth, &correct_uvs, &correct_seam]()
    {
        // returns angle above the XZ plane
        const auto inclination = [](const Vec3& v) -> Radians
        {
            return atan2(-v.y, length(Vec2{v.x, v.z}));
        };

        for (const Vec3& v : generated_vertices) {
            uvs.emplace_back(
                Turns{azimuth(v) + 0.5_turn}.count(),
                Turns{2.0f*inclination(v) + 0.5_turn}.count()
            );
        }

        correct_uvs();
        correct_seam();
    };

    subdivide(detail_level);
    apply_radius(radius);
    generate_uvs();

    OSC_ASSERT(generated_vertices.size() == uvs.size());
    OSC_ASSERT(generated_vertices.size() % 3 == 0);

    std::vector<uint32_t> generated_indices;
    generated_indices.reserve(generated_vertices.size());
    for (uint32_t i = 0; i < static_cast<uint32_t>(generated_vertices.size()); ++i) {
        generated_indices.push_back(i);
    }

    mesh_.setVerts(generated_vertices);
    mesh_.setTexCoords(uvs);
    mesh_.setIndices(generated_indices);
    if (detail_level == 0) {
        // flat-shade
        mesh_.recalculateNormals();
    }
    else {
        // smooth-shade
        auto normals{generated_vertices};
        for (auto& v : normals) {
            v = normalize(v);
        }
        mesh_.setNormals(normals);
    }
}
