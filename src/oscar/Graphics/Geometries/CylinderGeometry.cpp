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

Mesh osc::CylinderGeometry::generate_mesh(
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

    auto const fradialSegments = static_cast<float>(radialSegments);
    auto const fheightSegments = static_cast<float>(heightSegments);

    std::vector<uint32_t> indices;
    std::vector<Vec3> vertices;
    std::vector<Vec3> normals;
    std::vector<Vec2> uvs;

    uint32_t index = 0;
    std::vector<std::vector<uint32_t>> indexArray;
    float const halfHeight = height/2.0f;
    size_t groupStart = 0;
    std::vector<SubMeshDescriptor> groups;

    auto const generateTorso = [&]()
    {
        // used to calculate normal
        float const slope = (radiusBottom - radiusTop) / height;

        // generate vertices, normals, and uvs
        size_t groupCount = 0;
        for (size_t y = 0; y <= heightSegments; ++y) {
            std::vector<uint32_t> indexRow;
            float const v = static_cast<float>(y)/fheightSegments;
            float const radius = v * (radiusBottom - radiusTop) + radiusTop;
            for (size_t x = 0; x <= radialSegments; ++x) {
                auto const fx = static_cast<float>(x);
                float const u = fx/fradialSegments;
                Radians const theta = u*thetaLength + thetaStart;
                float const sinTheta = sin(theta);
                float const cosTheta = cos(theta);

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
                auto const a = indexArray.at(y+0).at(x+0);
                auto const b = indexArray.at(y+1).at(x+0);
                auto const c = indexArray.at(y+1).at(x+1);
                auto const d = indexArray.at(y+0).at(x+1);
                indices.insert(indices.end(), {a, b, d});
                indices.insert(indices.end(), {b, c, d});
                groupCount += 6;
            }
        }

        groups.emplace_back(groupStart, groupCount, MeshTopology::Triangles);
        groupStart += groupCount;
    };

    auto const generateCap = [&](bool top)
    {
        size_t groupCount = 0;

        auto const radius = top ? radiusTop : radiusBottom;
        auto const sign = top ? 1.0f : -1.0f;

        // first, generate the center vertex data of the cap.
        // Because, the geometry needs one set of uvs per face
        // we must generate a center vertex per face/segment

        auto const centerIndexStart = index;  // save first center vertex
        for (size_t x = 1; x <= radialSegments; ++x) {
            vertices.emplace_back(0.0f, sign*halfHeight, 0.0f);
            normals.emplace_back(0.0f, sign, 0.0f);
            uvs.emplace_back(0.5f, 0.5f);
            ++index;
        }
        auto const centerIndexEnd = index;  // save last center vertex

        // generate surrounding vertices, normals, and uvs
        for (size_t x = 0; x <= radialSegments; ++x) {
            auto const fx = static_cast<float>(x);
            float const u = fx / fradialSegments;
            Radians const theta = u * thetaLength + thetaStart;
            float const cosTheta = cos(theta);
            float const sinTheta = sin(theta);

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
            auto const c = static_cast<uint32_t>(centerIndexStart + x);
            auto const i = static_cast<uint32_t>(centerIndexEnd + x);

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

    Mesh rv;
    rv.setVerts(vertices);
    rv.setNormals(normals);
    rv.setTexCoords(uvs);
    rv.setIndices(indices);
    rv.setSubmeshDescriptors(groups);
    return rv;
}
