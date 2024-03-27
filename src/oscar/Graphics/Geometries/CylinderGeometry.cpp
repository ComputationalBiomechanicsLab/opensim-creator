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
    float radiusTop,
    float radiusBottom,
    float height,
    size_t radialSegments,
    size_t heightSegments,
    bool openEnded,
    Radians thetaStart,
    Radians thetaLength)
{
    // this implementation was initially hand-ported from threejs (CylinderGeometry)

    const auto fradialSegments = static_cast<float>(radialSegments);
    const auto fheightSegments = static_cast<float>(heightSegments);

    std::vector<uint32_t> indices;
    std::vector<Vec3> vertices;
    std::vector<Vec3> normals;
    std::vector<Vec2> uvs;

    uint32_t index = 0;
    std::vector<std::vector<uint32_t>> indexArray;
    const float halfHeight = height/2.0f;
    size_t groupStart = 0;
    std::vector<SubMeshDescriptor> groups;

    const auto generateTorso = [&]()
    {
        // used to calculate normal
        const float slope = (radiusBottom - radiusTop) / height;

        // generate vertices, normals, and uvs
        size_t groupCount = 0;
        for (size_t y = 0; y <= heightSegments; ++y) {
            std::vector<uint32_t> indexRow;
            const float v = static_cast<float>(y)/fheightSegments;
            const float radius = v * (radiusBottom - radiusTop) + radiusTop;
            for (size_t x = 0; x <= radialSegments; ++x) {
                const auto fx = static_cast<float>(x);
                const float u = fx/fradialSegments;
                const Radians theta = u*thetaLength + thetaStart;
                const float sinTheta = sin(theta);
                const float cosTheta = cos(theta);

                vertices.emplace_back(
                    radius * sinTheta,
                    (-v * height) + halfHeight,
                    radius * cosTheta
                );
                normals.push_back(UnitVec3{sinTheta, slope, cosTheta});
                uvs.emplace_back(u, 1 - v);
                indexRow.push_back(index++);
            }
            indexArray.push_back(std::move(indexRow));
        }

        // generate indices
        for (size_t x = 0; x < radialSegments; ++x) {
            for (size_t y = 0; y < heightSegments; ++y) {
                const auto a = indexArray.at(y+0).at(x+0);
                const auto b = indexArray.at(y+1).at(x+0);
                const auto c = indexArray.at(y+1).at(x+1);
                const auto d = indexArray.at(y+0).at(x+1);
                indices.insert(indices.end(), {a, b, d});
                indices.insert(indices.end(), {b, c, d});
                groupCount += 6;
            }
        }

        groups.emplace_back(groupStart, groupCount, MeshTopology::Triangles);
        groupStart += groupCount;
    };

    const auto generateCap = [&](bool top)
    {
        size_t groupCount = 0;

        const auto radius = top ? radiusTop : radiusBottom;
        const auto sign = top ? 1.0f : -1.0f;

        // first, generate the center vertex data of the cap.
        // Because, the geometry needs one set of uvs per face
        // we must generate a center vertex per face/segment

        const auto centerIndexStart = index;  // save first center vertex
        for (size_t x = 1; x <= radialSegments; ++x) {
            vertices.emplace_back(0.0f, sign*halfHeight, 0.0f);
            normals.emplace_back(0.0f, sign, 0.0f);
            uvs.emplace_back(0.5f, 0.5f);
            ++index;
        }
        const auto centerIndexEnd = index;  // save last center vertex

        // generate surrounding vertices, normals, and uvs
        for (size_t x = 0; x <= radialSegments; ++x) {
            const auto fx = static_cast<float>(x);
            const float u = fx / fradialSegments;
            const Radians theta = u * thetaLength + thetaStart;
            const float cosTheta = cos(theta);
            const float sinTheta = sin(theta);

            vertices.emplace_back(radius * sinTheta, halfHeight * sign, radius * cosTheta);
            normals.emplace_back(0.0f, sign, 0.0f);
            uvs.emplace_back(
                (cosTheta * 0.5f) + 0.5f,
                (sinTheta * 0.5f * sign) + 0.5f
            );
            ++index;
        }

        // generate indices
        for (size_t x = 0; x < radialSegments; ++x) {
            const auto c = static_cast<uint32_t>(centerIndexStart + x);
            const auto i = static_cast<uint32_t>(centerIndexEnd + x);

            if (top) {
                indices.insert(indices.end(), {i, i+1, c});
            }
            else {
                indices.insert(indices.end(), {i+1, i, c});
            }
            groupCount += 3;
        }

        groups.emplace_back(groupStart, groupCount, MeshTopology::Triangles);
        groupStart += groupCount;
    };

    generateTorso();
    if (!openEnded) {
        if (radiusTop > 0.0f) {
            generateCap(true);
        }
        if (radiusBottom > 0.0f) {
            generateCap(false);
        }
    }

    m_Mesh.setVerts(vertices);
    m_Mesh.setNormals(normals);
    m_Mesh.setTexCoords(uvs);
    m_Mesh.setIndices(indices);
    m_Mesh.setSubmeshDescriptors(groups);
}
