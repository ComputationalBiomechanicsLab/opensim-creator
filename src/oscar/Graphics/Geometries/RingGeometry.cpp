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

osc::RingGeometry::RingGeometry(
    float innerRadius,
    float outerRadius,
    size_t thetaSegments,
    size_t phiSegments,
    Radians thetaStart,
    Radians thetaLength)
{
    // this implementation was initially hand-ported from threejs (RingGeometry)

    thetaSegments = max(3_uz, thetaSegments);
    phiSegments = max(1_uz, phiSegments);
    const auto fthetaSegments = static_cast<float>(thetaSegments);
    const auto fphiSegments = static_cast<float>(phiSegments);

    std::vector<uint32_t> indices;
    std::vector<Vec3> vertices;
    std::vector<Vec3> normals;
    std::vector<Vec2> uvs;

    float radius = innerRadius;
    float radiusStep = (outerRadius - innerRadius)/fphiSegments;

    // generate vertices, normals, and uvs
    for (size_t j = 0; j <= phiSegments; ++j) {
        for (size_t i = 0; i <= thetaSegments; ++i) {
            const auto fi = static_cast<float>(i);
            Radians segment = thetaStart + (fi/fthetaSegments * thetaLength);

            const Vec3& v = vertices.emplace_back(radius * cos(segment), radius * sin(segment), 0.0f);
            normals.emplace_back(0.0f, 0.0f, 1.0f);
            uvs.emplace_back(
                (v.x/outerRadius + 1.0f) / 2.0f,
                (v.y/outerRadius + 1.0f) / 2.0f
            );
        }
        radius += radiusStep;
    }

    for (size_t j = 0; j < phiSegments; ++j) {
        const size_t thetaSegmentLevel = j * (thetaSegments + 1);
        for (size_t i = 0; i < thetaSegments; ++i) {
            size_t segment = i + thetaSegmentLevel;

            const auto a = static_cast<uint32_t>(segment);
            const auto b = static_cast<uint32_t>(segment + thetaSegments + 1);
            const auto c = static_cast<uint32_t>(segment + thetaSegments + 2);
            const auto d = static_cast<uint32_t>(segment + 1);

            indices.insert(indices.end(), {a, b, d});
            indices.insert(indices.end(), {b, c, d});
        }
    }

    m_Mesh.setVerts(vertices);
    m_Mesh.setNormals(normals);
    m_Mesh.setTexCoords(uvs);
    m_Mesh.setIndices(indices);
}
