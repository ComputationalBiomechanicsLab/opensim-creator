#include "MeshGenerators.h"

#include <oscar/Graphics/Geometries.h>
#include <oscar/Graphics/Mesh.h>
#include <oscar/Graphics/SubMeshDescriptor.h>
#include <oscar/Maths/Angle.h>
#include <oscar/Maths/CommonFunctions.h>
#include <oscar/Maths/LineSegment.h>
#include <oscar/Maths/MathHelpers.h>
#include <oscar/Maths/Triangle.h>
#include <oscar/Maths/TriangleFunctions.h>
#include <oscar/Maths/UnitVec3.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/Shims/Cpp23/cstddef.h>
#include <oscar/Utils/Assertions.h>
#include <oscar/Utils/At.h>
#include <oscar/Utils/EnumHelpers.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <numeric>
#include <span>
#include <unordered_set>
#include <vector>

using namespace osc::literals;
using namespace osc;

Mesh osc::GenerateGridLinesMesh(size_t n)
{
    constexpr float z = 0.0f;
    constexpr float min = -1.0f;
    constexpr float max = 1.0f;

    float const stepSize = (max - min) / static_cast<float>(n);

    size_t const nlines = n + 1;

    std::vector<Vec3> vertices;
    vertices.reserve(4 * nlines);
    std::vector<uint32_t> indices;
    indices.reserve(4 * nlines);
    std::vector<Vec3> normals;
    normals.reserve(4 * nlines);
    uint32_t index = 0;

    auto push = [&index, &vertices, &indices, &normals](Vec3 const& pos)
    {
        vertices.push_back(pos);
        indices.push_back(index++);
        normals.emplace_back(0.0f, 0.0f, 1.0f);
    };

    // lines parallel to X axis
    for (size_t i = 0; i < nlines; ++i)
    {
        float const y = min + static_cast<float>(i) * stepSize;

        push({-1.0f, y, z});
        push({+1.0f, y, z});
    }

    // lines parallel to Y axis
    for (size_t i = 0; i < nlines; ++i)
    {
        float const x = min + static_cast<float>(i) * stepSize;

        push({x, -1.0f, z});
        push({x, +1.0f, z});
    }

    Mesh rv;
    rv.setTopology(MeshTopology::Lines);
    rv.setVerts(vertices);
    rv.setNormals(normals);
    rv.setIndices(indices);
    return rv;
}

Mesh osc::GenerateYToYLineMesh()
{
    Mesh rv;
    rv.setTopology(MeshTopology::Lines);
    rv.setVerts({{0.0f, -1.0f, 0.0f}, {0.0f, +1.0f, 0.0f}});
    rv.setNormals({{0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}});  // just give them *something* in-case they are rendered through a shader that requires normals
    rv.setIndices({0, 1});
    return rv;
}

Mesh osc::GenerateCubeLinesMesh()
{
    Vec3 min = {-1.0f, -1.0f, -1.0f};
    Vec3 max = { 1.0f,  1.0f,  1.0f};

    Mesh rv;
    rv.setTopology(MeshTopology::Lines);
    rv.setVerts({
        {max.x, max.y, max.z},
        {min.x, max.y, max.z},
        {min.x, min.y, max.z},
        {max.x, min.y, max.z},
        {max.x, max.y, min.z},
        {min.x, max.y, min.z},
        {min.x, min.y, min.z},
        {max.x, min.y, min.z},
    });
    rv.setIndices({0, 1, 1, 2, 2, 3, 3, 0, 4, 5, 5, 6, 6, 7, 7, 4, 0, 4, 1, 5, 2, 6, 3, 7});
    return rv;
}

Mesh osc::GenerateTorusKnotMesh(
    float torusRadius,
    float tubeRadius,
    size_t numTubularSegments,
    size_t numRadialSegments,
    size_t p,
    size_t q)
{
    return TorusKnotGeometry::generate_mesh(
        torusRadius,
        tubeRadius,
        numTubularSegments,
        numRadialSegments,
        p,
        q
    );
}

Mesh osc::GenerateBoxMesh(
    float width,
    float height,
    float depth,
    size_t widthSegments,
    size_t heightSegments,
    size_t depthSegments)
{
    return BoxGeometry::generate_mesh(
        width,
        height,
        depth,
        widthSegments,
        heightSegments,
        depthSegments
    );
}

Mesh osc::GeneratePolyhedronMesh(
    std::span<Vec3 const> vertices,
    std::span<uint32_t const> indices,
    float radius,
    size_t detail)
{
    return PolyhedronGeometry::generate_mesh(
        vertices,
        indices,
        radius,
        detail
    );
}

Mesh osc::GenerateIcosahedronMesh(
    float radius,
    size_t detail)
{
    return IcosahedronGeometry::generate_mesh(radius, detail);
}

Mesh osc::GenerateDodecahedronMesh(
    float radius,
    size_t detail)
{
    return DodecahedronGeometry::generate_mesh(radius, detail);
}

Mesh osc::GenerateOctahedronMesh(
    float radius,
    size_t detail)
{
    return OctahedronGeometry::generate_mesh(radius, detail);
}

Mesh osc::GenerateTetrahedronMesh(
    float radius,
    size_t detail)
{
    return TetrahedronGeometry::generate_mesh(radius, detail);
}

Mesh osc::GenerateLatheMesh(
    std::span<Vec2 const> points,
    size_t segments,
    Radians phiStart,
    Radians phiLength)
{
    return LatheGeometry::generate_mesh(points, segments, phiStart, phiLength);
}

Mesh osc::GenerateCircleMesh(
    float radius,
    size_t segments,
    Radians thetaStart,
    Radians thetaLength)
{
    return CircleGeometry::generate_mesh(radius, segments, thetaStart, thetaLength);
}

Mesh osc::GenerateRingMesh(
    float innerRadius,
    float outerRadius,
    size_t thetaSegments,
    size_t phiSegments,
    Radians thetaStart,
    Radians thetaLength)
{
    return RingGeometry::generate_mesh(innerRadius, outerRadius, thetaSegments, phiSegments, thetaStart, thetaLength);
}

Mesh osc::GenerateTorusMesh(
    float radius,
    float tube,
    size_t radialSegments,
    size_t tubularSegments,
    Radians arc)
{
    return TorusGeometry::generate_mesh(radius, tube, radialSegments, tubularSegments, arc);
}

Mesh osc::GenerateCylinderMesh(
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

Mesh osc::GenerateConeMesh(
    float radius,
    float height,
    size_t radialSegments,
    size_t heightSegments,
    bool openEnded,
    Radians thetaStart,
    Radians thetaLength)
{
    return GenerateCylinderMesh(
        0.0f,
        radius,
        height,
        radialSegments,
        heightSegments,
        openEnded,
        thetaStart,
        thetaLength
    );
}

Mesh osc::GeneratePlaneMesh(
    float width,
    float height,
    size_t widthSegments,
    size_t heightSegments)
{
    float const halfWidth = width/2.0f;
    float const halfHeight = height/2.0f;
    size_t const gridX = widthSegments;
    size_t const gridY = heightSegments;
    size_t const gridX1 = gridX + 1;
    size_t const gridY1 = gridY + 1;
    float const segmentWidth = width / static_cast<float>(gridX);
    float const segmentHeight = height / static_cast<float>(gridY);

    std::vector<uint32_t> indices;
    std::vector<Vec3> vertices;
    std::vector<Vec3> normals;
    std::vector<Vec2> uvs;

    // generate vertices, normals, and uvs
    for (size_t iy = 0; iy < gridY1; ++iy) {
        float const y = static_cast<float>(iy) * segmentHeight - halfHeight;
        for (size_t ix = 0; ix < gridX1; ++ix) {
            float const x = static_cast<float>(ix) * segmentWidth - halfWidth;

            vertices.emplace_back(x, -y, 0.0f);
            normals.emplace_back(0.0f, 0.0f, 1.0f);
            uvs.emplace_back(
                       static_cast<float>(ix)/static_cast<float>(gridX),
                1.0f - static_cast<float>(iy)/static_cast<float>(gridY)
            );
        }
    }

    for (size_t iy = 0; iy < gridY; ++iy) {
        for (size_t ix = 0; ix < gridX; ++ix) {
            auto const a = static_cast<uint32_t>((ix + 0) + gridX1*(iy + 0));
            auto const b = static_cast<uint32_t>((ix + 0) + gridX1*(iy + 1));
            auto const c = static_cast<uint32_t>((ix + 1) + gridX1*(iy + 1));
            auto const d = static_cast<uint32_t>((ix + 1) + gridX1*(iy + 0));
            indices.insert(indices.end(), {a, b, d});
            indices.insert(indices.end(), {b, c, d});
        }
    }

    Mesh m;
    m.setVerts(vertices);
    m.setNormals(normals);
    m.setTexCoords(uvs);
    m.setIndices(indices);
    return m;
}

Mesh osc::GenerateSphereMesh(
    float radius,
    size_t widthSegments,
    size_t heightSegments,
    Radians phiStart,
    Radians phiLength,
    Radians thetaStart,
    Radians thetaLength)
{
    // implementation was initially hand-ported from three.js (SphereGeometry)

    widthSegments = max(3_uz, widthSegments);
    heightSegments = max(2_uz, heightSegments);
    auto const fwidthSegments = static_cast<float>(widthSegments);
    auto const fheightSegments = static_cast<float>(heightSegments);
    auto const thetaEnd = min(thetaStart + thetaLength, 180_deg);

    uint32_t index = 0;
    std::vector<std::vector<uint32_t>> grid;

    std::vector<uint32_t> indices;
    std::vector<Vec3> vertices;
    std::vector<Vec3> normals;
    std::vector<Vec2> uvs;

    // generate vertices, normals, and uvs
    for (size_t iy = 0; iy <= heightSegments; ++iy) {
        std::vector<uint32_t> verticesRow;
        float const v = static_cast<float>(iy) / fheightSegments;

        // edge-case: poles
        float uOffset = 0.0f;
        if (iy == 0 && thetaStart == 0_deg) {
            uOffset = 0.5f / fwidthSegments;
        }
        else if (iy == heightSegments && thetaEnd == 180_deg) {
            uOffset = -0.5f / fwidthSegments;
        }

        for (size_t ix = 0; ix <= widthSegments; ++ix) {
            float const u = static_cast<float>(ix) / fwidthSegments;

            Vec3 const& vertex = vertices.emplace_back(
                -radius * cos(phiStart + u*phiLength) * sin(thetaStart + v*thetaLength),
                 radius * cos(thetaStart + v*thetaLength),
                 radius * sin(phiStart + u*phiLength) * sin(thetaStart + v*thetaLength)
            );
            normals.push_back(normalize(vertex));
            uvs.emplace_back(u + uOffset, 1.0f - v);

            verticesRow.push_back(index++);
        }
        grid.push_back(std::move(verticesRow));
    }

    // generate indices
    for (size_t iy = 0; iy < heightSegments; ++iy) {
        for (size_t ix = 0; ix < widthSegments; ++ix) {
            uint32_t const a = grid.at(iy  ).at(ix+1);
            uint32_t const b = grid.at(iy  ).at(ix);
            uint32_t const c = grid.at(iy+1).at(ix);
            uint32_t const d = grid.at(iy+1).at(ix+1);

            if (iy != 0 || thetaStart > 0_deg) {
                indices.insert(indices.end(), {a, b, d});
            }
            if (iy != (heightSegments-1) || thetaEnd < 180_deg) {
                indices.insert(indices.end(), {b, c, d});
            }
        }
    }

    Mesh rv;
    rv.setVerts(vertices);
    rv.setNormals(normals);
    rv.setTexCoords(uvs);
    rv.setIndices(indices);
    return rv;
}

Mesh osc::GenerateWireframeMesh(Mesh const& mesh)
{
    static_assert(NumOptions<MeshTopology>() == 2);

    if (mesh.getTopology() == MeshTopology::Lines) {
        return mesh;
    }

    std::unordered_set<LineSegment> edges;
    edges.reserve(mesh.getNumIndices());  // (guess)

    std::vector<Vec3> points;
    points.reserve(mesh.getNumIndices());  // (guess)

    mesh.forEachIndexedTriangle([&edges, &points](Triangle const& triangle)
    {
        auto [a, b, c] = triangle;

        auto const orderedEdge = [](Vec3 p1, Vec3 p2)
        {
            return lexicographical_compare(p1, p2) ? LineSegment{p1, p2} : LineSegment{p2, p1};
        };

        if (auto ab = orderedEdge(a, b); edges.emplace(ab).second) {
            points.insert(points.end(), {ab.p1, ab.p2});
        }

        if (auto ac = orderedEdge(a, c); edges.emplace(ac).second) {
            points.insert(points.end(), {ac.p1, ac.p2});
        }

        if (auto bc = orderedEdge(b, c); edges.emplace(bc).second) {
            points.insert(points.end(), {bc.p1, bc.p2});
        }
    });

    std::vector<uint32_t> indices;
    indices.reserve(points.size());
    for (size_t i = 0; i < points.size(); ++i) {
        indices.push_back(static_cast<uint32_t>(i));
    }

    Mesh rv;
    rv.setTopology(MeshTopology::Lines);
    rv.setVerts(points);
    rv.setIndices(indices);
    return rv;
}
