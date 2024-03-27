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
    size_t detail)
{
    std::vector<Vec3> vertexBuffer;
    std::vector<Vec2> uvBuffer;

    const auto subdivideFace = [&vertexBuffer](Vec3 a, Vec3 b, Vec3 c, size_t detail)
    {
        const auto cols = detail + 1;
        const auto fcols = static_cast<float>(cols);

        // we use this multidimensional array as a data structure for creating the subdivision
        std::vector<std::vector<Vec3>> v;
        v.reserve(cols+1);

        for (size_t i = 0; i <= cols; ++i) {
            const auto fi = static_cast<float>(i);
            const Vec3 aj = lerp(a, c, fi/fcols);
            const Vec3 bj = lerp(b, c, fi/fcols);

            const auto rows = cols - i;
            const auto frows = static_cast<float>(rows);

            v.emplace_back().reserve(rows+1);
            for (size_t j = 0; j <= rows; ++j) {
                v.at(i).emplace_back();

                const auto fj = static_cast<float>(j);
                if (j == 0 && i == cols) {
                    v.at(i).at(j) = aj;
                }
                else {
                    v.at(i).at(j) = lerp(aj, bj, fj/frows);
                }
            }
        }

        // construct all of the faces
        for (size_t i = 0; i < cols; ++i) {
            for (size_t j = 0; j < 2*(cols-i) - 1; ++j) {
                const size_t k = j/2;

                if (j % 2 == 0) {
                    vertexBuffer.insert(vertexBuffer.end(), {
                        v.at(i).at(k+1),
                        v.at(i+1).at(k),
                        v.at(i).at(k),
                    });
                }
                else {
                    vertexBuffer.insert(vertexBuffer.end(), {
                        v.at(i).at(k+1),
                        v.at(i+1).at(k+1),
                        v.at(i+1).at(k),
                    });
                }
            }
        }
    };

    const auto subdivide = [&subdivideFace, &vertices, &indices](size_t detail)
    {
        // subdivide each input triangle by the given detail
        for (size_t i = 0; i < 3*(indices.size()/3); i += 3) {
            const Vec3 a = at(vertices, at(indices, i+0));
            const Vec3 b = at(vertices, at(indices, i+1));
            const Vec3 c = at(vertices, at(indices, i+2));
            subdivideFace(a, b, c, detail);
        }
    };

    const auto applyRadius = [&vertexBuffer](float radius)
    {
        for (Vec3& v : vertexBuffer) {
            v = radius * normalize(v);
        }
    };

    // return the angle around the Y axis, CCW when looking from above
    const auto azimuth = [](const Vec3& v) -> Radians
    {
        return atan2(v.z, -v.x);
    };

    const auto correctUV = [](Vec2& uv, const Vec3& vector, Radians azimuth)
    {
        if ((azimuth < 0_rad) && (uv.x == 1.0f)) {
            uv.x -= 1.0f;
        }
        if ((vector.x == 0.0f) && (vector.z == 0.0f)) {
            uv.x = Turns{azimuth + 0.5_turn}.count();
        }
    };

    const auto correctUVs = [&vertexBuffer, &uvBuffer, &azimuth, &correctUV]()
    {
        OSC_ASSERT(vertexBuffer.size() == uvBuffer.size());
        OSC_ASSERT(vertexBuffer.size() % 3 == 0);

        for (size_t i = 0; i < 3*(vertexBuffer.size()/3); i += 3) {
            const Vec3 a = vertexBuffer[i+0];
            const Vec3 b = vertexBuffer[i+1];
            const Vec3 c = vertexBuffer[i+2];

            const auto azi = azimuth(centroid({a, b, c}));

            correctUV(uvBuffer[i+0], a, azi);
            correctUV(uvBuffer[i+1], b, azi);
            correctUV(uvBuffer[i+2], c, azi);
        }
    };

    const auto correctSeam = [&uvBuffer]()
    {
        // handle case when face straddles the seam, see mrdoob/three.js#3269
        OSC_ASSERT(uvBuffer.size() % 3 == 0);
        for (size_t i = 0; i < 3*(uvBuffer.size()/3); i += 3) {
            const float x0 = uvBuffer[i+0].x;
            const float x1 = uvBuffer[i+1].x;
            const float x2 = uvBuffer[i+2].x;
            const auto [min, max] = minmax({x0, x1, x2});

            // these magic numbers are arbitrary (copied from three.js)
            if (max > 0.9f && min < 0.1f) {
                if (x0 < 0.2f) { uvBuffer[i+0] += 1.0f; }
                if (x1 < 0.2f) { uvBuffer[i+1] += 1.0f; }
                if (x2 < 0.2f) { uvBuffer[i+2] += 1.0f; }
            }
        }
    };

    const auto generateUVs = [&vertexBuffer, &uvBuffer, &azimuth, &correctUVs, &correctSeam]()
    {
        // returns angle above the XZ plane
        const auto inclination = [](const Vec3& v) -> Radians
        {
            return atan2(-v.y, length(Vec2{v.x, v.z}));
        };

        for (const Vec3& v : vertexBuffer) {
            uvBuffer.emplace_back(
                Turns{azimuth(v) + 0.5_turn}.count(),
                Turns{2.0f*inclination(v) + 0.5_turn}.count()
            );
        }

        correctUVs();
        correctSeam();
    };

    subdivide(detail);
    applyRadius(radius);
    generateUVs();

    OSC_ASSERT(vertexBuffer.size() == uvBuffer.size());
    OSC_ASSERT(vertexBuffer.size() % 3 == 0);

    std::vector<uint32_t> meshIndices;
    meshIndices.reserve(vertexBuffer.size());
    for (uint32_t i = 0; i < static_cast<uint32_t>(vertexBuffer.size()); ++i) {
        meshIndices.push_back(i);
    }

    m_Mesh.setVerts(vertexBuffer);
    m_Mesh.setTexCoords(uvBuffer);
    m_Mesh.setIndices(meshIndices);
    if (detail == 0) {
        m_Mesh.recalculateNormals();  // flat-shaded
    }
    else {
        auto meshNormals = vertexBuffer;
        for (auto& v : meshNormals) { v = normalize(v); }
        m_Mesh.setNormals(meshNormals);
    }
}
