#include "MeshGenerators.h"

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

namespace
{
    struct UntexturedVert final {
        Vec3 pos;
        Vec3 norm;
    };

    // a cube wire mesh, suitable for `MeshTopology::Lines` drawing
    //
    // a pair of verts per edge of the cube. The cube has 12 edges, so 24 lines
    constexpr std::array<UntexturedVert, 24> c_CubeEdgeLines =
    {{
        // back

        // back bottom left -> back bottom right
        {{-1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}},
        {{+1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}},

        // back bottom right -> back top right
        {{+1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}},
        {{+1.0f, +1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}},

        // back top right -> back top left
        {{+1.0f, +1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}},
        {{-1.0f, +1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}},

        // back top left -> back bottom left
        {{-1.0f, +1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}},
        {{-1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}},

        // front

        // front bottom left -> front bottom right
        {{-1.0f, -1.0f, +1.0f}, {0.0f, 0.0f, +1.0f}},
        {{+1.0f, -1.0f, +1.0f}, {0.0f, 0.0f, +1.0f}},

        // front bottom right -> front top right
        {{+1.0f, -1.0f, +1.0f}, {0.0f, 0.0f, +1.0f}},
        {{+1.0f, +1.0f, +1.0f}, {0.0f, 0.0f, +1.0f}},

        // front top right -> front top left
        {{+1.0f, +1.0f, +1.0f}, {0.0f, 0.0f, +1.0f}},
        {{-1.0f, +1.0f, +1.0f}, {0.0f, 0.0f, +1.0f}},

        // front top left -> front bottom left
        {{-1.0f, +1.0f, +1.0f}, {0.0f, 0.0f, +1.0f}},
        {{-1.0f, -1.0f, +1.0f}, {0.0f, 0.0f, +1.0f}},

        // front-to-back edges

        // front bottom left -> back bottom left
        {{-1.0f, -1.0f, +1.0f}, {-1.0f, -1.0f, +1.0f}},
        {{-1.0f, -1.0f, -1.0f}, {-1.0f, -1.0f, -1.0f}},

        // front bottom right -> back bottom right
        {{+1.0f, -1.0f, +1.0f}, {+1.0f, -1.0f, +1.0f}},
        {{+1.0f, -1.0f, -1.0f}, {+1.0f, -1.0f, -1.0f}},

        // front top left -> back top left
        {{-1.0f, +1.0f, +1.0f}, {-1.0f, +1.0f, +1.0f}},
        {{-1.0f, +1.0f, -1.0f}, {-1.0f, +1.0f, -1.0f}},

        // front top right -> back top right
        {{+1.0f, +1.0f, +1.0f}, {+1.0f, +1.0f, +1.0f}},
        {{+1.0f, +1.0f, -1.0f}, {+1.0f, +1.0f, -1.0f}}
    }};

    struct NewMeshData final {

        void clear()
        {
            verts.clear();
            normals.clear();
            texcoords.clear();
            indices.clear();
            topology = MeshTopology::Triangles;
        }

        void reserve(size_t s)
        {
            verts.reserve(s);
            normals.reserve(s);
            texcoords.reserve(s);
            indices.reserve(s);
        }

        std::vector<Vec3> verts;
        std::vector<Vec3> normals;
        std::vector<Vec2> texcoords;
        std::vector<uint32_t> indices;
        MeshTopology topology = MeshTopology::Triangles;
    };

    Mesh CreateMeshFromData(NewMeshData&& data)
    {
        Mesh rv;
        rv.setTopology(data.topology);
        rv.setVerts(data.verts);
        rv.setNormals(data.normals);
        rv.setTexCoords(data.texcoords);
        rv.setIndices(data.indices);
        return rv;
    }
}

Mesh osc::GenerateGridLinesMesh(size_t n)
{
    constexpr float z = 0.0f;
    constexpr float min = -1.0f;
    constexpr float max = 1.0f;

    float const stepSize = (max - min) / static_cast<float>(n);

    size_t const nlines = n + 1;

    NewMeshData data;
    data.reserve(4 * nlines);
    data.topology = MeshTopology::Lines;

    uint16_t index = 0;
    auto push = [&index, &data](Vec3 const& pos)
    {
        data.verts.push_back(pos);
        data.indices.push_back(index++);
        data.normals.emplace_back(0.0f, 0.0f, 1.0f);
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

    OSC_ASSERT(data.verts.size() % 2 == 0);  // lines, not triangles
    OSC_ASSERT(data.normals.size() == data.verts.size());  // they contain dummy normals
    OSC_ASSERT(data.verts.size() == data.indices.size());

    return CreateMeshFromData(std::move(data));
}

Mesh osc::GenerateYToYLineMesh()
{
    NewMeshData data;
    data.verts = {{0.0f, -1.0f, 0.0f}, {0.0f, +1.0f, 0.0f}};
    data.normals = {{0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}};  // just give them *something* in-case they are rendered through a shader that requires normals
    data.indices = {0, 1};
    data.topology = MeshTopology::Lines;

    OSC_ASSERT(data.verts.size() % 2 == 0);
    OSC_ASSERT(data.normals.size() % 2 == 0);
    OSC_ASSERT(data.verts.size() == data.indices.size());

    return CreateMeshFromData(std::move(data));
}

Mesh osc::GenerateCubeLinesMesh()
{
    NewMeshData data;
    data.verts.reserve(c_CubeEdgeLines.size());
    data.indices.reserve(c_CubeEdgeLines.size());
    data.topology = MeshTopology::Lines;

    uint16_t index = 0;
    for (UntexturedVert const& v : c_CubeEdgeLines)
    {
        data.verts.push_back(v.pos);
        data.indices.push_back(index++);
    }

    OSC_ASSERT(data.verts.size() % 2 == 0);  // lines, not triangles
    OSC_ASSERT(data.normals.empty());
    OSC_ASSERT(data.verts.size() == data.indices.size());

    return CreateMeshFromData(std::move(data));
}

Mesh osc::GenerateTorusKnotMesh(
    float torusRadius,
    float tubeRadius,
    size_t numTubularSegments,
    size_t numRadialSegments,
    size_t p,
    size_t q)
{
    // the implementation/API of this was initially translated from `three.js`'s
    // `TorusKnotGeometry`, which has excellent documentation and source code. The
    // code was then subsequently mutated to suit OSC, C++ etc.
    //
    // https://threejs.org/docs/#api/en/geometries/TorusKnotGeometry

    auto const fNumTubularSegments = static_cast<float>(numTubularSegments);
    auto const fNumRadialSegments = static_cast<float>(numRadialSegments);
    auto const fp = static_cast<float>(p);
    auto const fq = static_cast<float>(q);

    // helper: calculates the current position on the torus curve
    auto const calculatePositionOnCurve = [fp, fq, torusRadius](Radians u)
    {
        Radians const quOverP = fq/fp * u;
        float const cs = cos(quOverP);

        return Vec3{
            torusRadius * (2.0f + cs) * 0.5f * cos(u),
            torusRadius * (2.0f + cs) * 0.5f * sin(u),
            torusRadius * sin(quOverP) * 0.5f,
        };
    };

    size_t const numVerts = (numTubularSegments+1)*(numRadialSegments+1);
    size_t const numIndices = 6*numTubularSegments*numRadialSegments;

    std::vector<uint32_t> indices;
    indices.reserve(numIndices);
    std::vector<Vec3> vertices;
    vertices.reserve(numVerts);
    std::vector<Vec3> normals;
    normals.reserve(numVerts);
    std::vector<Vec2> uvs;
    uvs.reserve(numVerts);

    // generate vertices, normals, and uvs
    for (size_t i = 0; i <= numTubularSegments; ++i) {
        auto const fi = static_cast<float>(i);

        // `u` is used to calculate the position on the torus curve of the current tubular segment
        Radians const u = fi/fNumTubularSegments * fp * 360_deg;

        // now we calculate two points. P1 is our current position on the curve, P2 is a little farther ahead.
        // these points are used to create a special "coordinate space", which is necessary to calculate the
        // correct vertex positions
        Vec3 const P1 = calculatePositionOnCurve(u);
        Vec3 const P2 = calculatePositionOnCurve(u + 0.01_rad);

        // calculate orthonormal basis
        Vec3 const T = P2 - P1;
        Vec3 N = P2 + P1;
        Vec3 B = cross(T, N);
        N = cross(B, T);

        // normalize B, N. T can be ignored, we don't use it
        B = normalize(B);
        N = normalize(N);

        for (size_t j = 0; j <= numRadialSegments; ++j) {
            auto const fj = static_cast<float>(j);

            // now calculate the vertices. they are nothing more than an extrusion of the torus curve.
            // because we extrude a shape in the xy-plane, there is no need to calculate a z-value.

            Radians const v = fj/fNumRadialSegments * 360_deg;
            float const cx = -tubeRadius * cos(v);
            float const cy =  tubeRadius * sin(v);

            // now calculate the final vertex position.
            // first we orient the extrusion with our basis vectors, then we add it to the current position on the curve
            Vec3 const vertex = {
                P1.x + (cx * N.x + cy * B.x),
                P1.y + (cx * N.y + cy * B.y),
                P1.z + (cx * N.z + cy * B.z),
            };
            vertices.push_back(vertex);

            // normal (P1 is always the center/origin of the extrusion, thus we can use it to calculate the normal)
            normals.push_back(normalize(vertex - P1));

            uvs.emplace_back(fi / fNumTubularSegments, fj / fNumRadialSegments);
        }
    }

    // generate indices
    for (size_t j = 1; j <= numTubularSegments; ++j) {
        for (size_t i = 1; i <= numRadialSegments; ++i) {
            auto const a = static_cast<uint32_t>((numRadialSegments + 1) * (j - 1) + (i - 1));
            auto const b = static_cast<uint32_t>((numRadialSegments + 1) *  j      + (i - 1));
            auto const c = static_cast<uint32_t>((numRadialSegments + 1) *  j      +  i);
            auto const d = static_cast<uint32_t>((numRadialSegments + 1) * (j - 1) +  i);

            indices.insert(indices.end(), {a, b, d});
            indices.insert(indices.end(), {b, c, d});
        }
    }

    // build geometry
    Mesh rv;
    rv.setVerts(vertices);
    rv.setNormals(normals);
    rv.setTexCoords(uvs);
    rv.setIndices(indices);
    return rv;
}

Mesh osc::GenerateBoxMesh(
    float width,
    float height,
    float depth,
    size_t widthSegments,
    size_t heightSegments,
    size_t depthSegments)
{
    // the implementation/API of this was initially translated from `three.js`'s
    // `BoxGeometry`, which has excellent documentation and source code. The
    // code was then subsequently mutated to suit OSC, C++ etc.
    //
    // https://threejs.org/docs/#api/en/geometries/BoxGeometry

    std::vector<uint32_t> indices;
    std::vector<Vec3> vertices;
    std::vector<Vec3> normals;
    std::vector<Vec2> uvs;
    std::vector<SubMeshDescriptor> submeshes;  // for multi-material support

    // helper variables
    size_t numberOfVertices = 0;
    size_t groupStart = 0;

    // helper function
    auto const buildPlane = [&indices, &vertices, &normals, &uvs, &submeshes, &numberOfVertices, &groupStart](
        Vec3::size_type u,
        Vec3::size_type v,
        Vec3::size_type w,
        float udir,
        float vdir,
        Vec3 dims,
        size_t gridX,
        size_t gridY)
    {
        float const segmentWidth = dims.x / static_cast<float>(gridX);
        float const segmentHeight = dims.y / static_cast<float>(gridY);

        float const widthHalf = 0.5f * dims.x;
        float const heightHalf = 0.5f * dims.y;
        float const depthHalf = 0.5f * dims.z;

        size_t const gridX1 = gridX + 1;
        size_t const gridY1 = gridY + 1;

        size_t vertexCount = 0;
        size_t groupCount = 0;

        // generate vertices, normals, and UVs
        for (size_t iy = 0; iy < gridY1; ++iy) {
            float const y = static_cast<float>(iy)*segmentHeight - heightHalf;
            for (size_t ix = 0; ix < gridX1; ++ix) {
                float const x = static_cast<float>(ix)*segmentWidth - widthHalf;

                Vec3 vertex{};
                vertex[u] = x*udir;
                vertex[v] = y*vdir;
                vertex[w] = depthHalf;
                vertices.push_back(vertex);

                Vec3 normal{};
                normal[u] = 0.0f;
                normal[v] = 0.0f;
                normal[w] = dims.z > 0.0f ? 1.0f : -1.0f;
                normals.push_back(normal);

                uvs.emplace_back(ix/gridX, 1 - (iy/gridY));

                ++vertexCount;
            }
        }

        // indices (two triangles, or 6 indices, per segment)
        for (size_t iy = 0; iy < gridY; ++iy) {
            for (size_t ix = 0; ix < gridX; ++ix) {
                auto const a = static_cast<uint32_t>(numberOfVertices +  ix      + (gridX1 *  iy     ));
                auto const b = static_cast<uint32_t>(numberOfVertices +  ix      + (gridX1 * (iy + 1)));
                auto const c = static_cast<uint32_t>(numberOfVertices + (ix + 1) + (gridX1 * (iy + 1)));
                auto const d = static_cast<uint32_t>(numberOfVertices + (ix + 1) + (gridX1 *  iy     ));

                indices.insert(indices.end(), {a, b, d});
                indices.insert(indices.end(), {b, c, d});

                groupCount += 6;
            }
        }

        // add submesh description
        submeshes.emplace_back(groupStart, groupCount, MeshTopology::Triangles);
        groupStart += groupCount;
        numberOfVertices += vertexCount;
    };

    // build each side of the box
    buildPlane(2, 1, 0, -1.0f, -1.0f, {depth, height,  width},  depthSegments, heightSegments);  // px
    buildPlane(2, 1, 0,  1.0f, -1.0f, {depth, height, -width},  depthSegments, heightSegments);  // nx
    buildPlane(0, 2, 1,  1.0f,  1.0f, {width, depth,   height}, widthSegments, depthSegments);   // py
    buildPlane(0, 2, 1,  1.0f, -1.0f, {width, depth,  -height}, widthSegments, depthSegments);   // ny
    buildPlane(0, 1, 2,  1.0f, -1.0f, {width, height,  depth},  widthSegments, heightSegments);  // pz
    buildPlane(0, 1, 2, -1.0f, -1.0f, {width, height, -depth},  widthSegments, heightSegments);  // nz

    // the first submesh is "the entire cube"
    submeshes.insert(submeshes.begin(), SubMeshDescriptor{0, groupStart, MeshTopology::Triangles});

    // build geometry
    Mesh rv;
    rv.setVerts(vertices);
    rv.setNormals(normals);
    rv.setTexCoords(uvs);
    rv.setIndices(indices);
    rv.setSubmeshDescriptors(submeshes);
    return rv;
}

Mesh osc::GeneratePolyhedronMesh(
    std::span<Vec3 const> vertices,
    std::span<uint32_t const> indices,
    float radius,
    size_t detail)
{
    std::vector<Vec3> vertexBuffer;
    std::vector<Vec2> uvBuffer;

    auto const subdivideFace = [&vertexBuffer](Vec3 a, Vec3 b, Vec3 c, size_t detail)
    {
        auto const cols = detail + 1;
        auto const fcols = static_cast<float>(cols);

        // we use this multidimensional array as a data structure for creating the subdivision
        std::vector<std::vector<Vec3>> v;
        v.reserve(cols+1);

        for (size_t i = 0; i <= cols; ++i) {
            auto const fi = static_cast<float>(i);
            Vec3 const aj = lerp(a, c, fi/fcols);
            Vec3 const bj = lerp(b, c, fi/fcols);

            auto const rows = cols - i;
            auto const frows = static_cast<float>(rows);

            v.emplace_back().reserve(rows+1);
            for (size_t j = 0; j <= rows; ++j) {
                v.at(i).emplace_back();

                auto const fj = static_cast<float>(j);
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
                size_t const k = j/2;

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

    auto const subdivide = [&subdivideFace, &vertices, &indices](size_t detail)
    {
        // subdivide each input triangle by the given detail
        for (size_t i = 0; i < 3*(indices.size()/3); i += 3) {
            Vec3 const a = At(vertices, At(indices, i+0));
            Vec3 const b = At(vertices, At(indices, i+1));
            Vec3 const c = At(vertices, At(indices, i+2));
            subdivideFace(a, b, c, detail);
        }
    };

    auto const applyRadius = [&vertexBuffer](float radius)
    {
        for (Vec3& v : vertexBuffer) {
            v = radius * normalize(v);
        }
    };

    // return the angle around the Y axis, CCW when looking from above
    auto const azimuth = [](Vec3 const& v) -> Radians
    {
        return atan2(v.z, -v.x);
    };

    auto const correctUV = [](Vec2& uv, Vec3 const& vector, Radians azimuth)
    {
        if ((azimuth < 0_rad) && (uv.x == 1.0f)) {
            uv.x -= 1.0f;
        }
        if ((vector.x == 0.0f) && (vector.z == 0.0f)) {
            uv.x = Turns{azimuth + 0.5_turn}.count();
        }
    };

    auto const correctUVs = [&vertexBuffer, &uvBuffer, &azimuth, &correctUV]()
    {
        OSC_ASSERT(vertexBuffer.size() == uvBuffer.size());
        OSC_ASSERT(vertexBuffer.size() % 3 == 0);

        for (size_t i = 0; i < 3*(vertexBuffer.size()/3); i += 3) {
            Vec3 const a = vertexBuffer[i+0];
            Vec3 const b = vertexBuffer[i+1];
            Vec3 const c = vertexBuffer[i+2];

            auto const azi = azimuth(centroid({a, b, c}));

            correctUV(uvBuffer[i+0], a, azi);
            correctUV(uvBuffer[i+1], b, azi);
            correctUV(uvBuffer[i+2], c, azi);
        }
    };

    auto const correctSeam = [&uvBuffer]()
    {
        // handle case when face straddles the seam, see mrdoob/three.js#3269
        OSC_ASSERT(uvBuffer.size() % 3 == 0);
        for (size_t i = 0; i < 3*(uvBuffer.size()/3); i += 3) {
            float const x0 = uvBuffer[i+0].x;
            float const x1 = uvBuffer[i+1].x;
            float const x2 = uvBuffer[i+2].x;
            auto const [min, max] = std::minmax({x0, x1, x2});

            // these magic numbers are arbitrary (copied from three.js)
            if (max > 0.9f && min < 0.1f) {
                if (x0 < 0.2f) { uvBuffer[i+0] += 1.0f; }
                if (x1 < 0.2f) { uvBuffer[i+1] += 1.0f; }
                if (x2 < 0.2f) { uvBuffer[i+2] += 1.0f; }
            }
        }
    };

    auto const generateUVs = [&vertexBuffer, &uvBuffer, &azimuth, &correctUVs, &correctSeam]()
    {
        // returns angle above the XZ plane
        auto const inclination = [](Vec3 const& v) -> Radians
        {
            return atan2(-v.y, length(Vec2{v.x, v.z}));
        };

        for (Vec3 const& v : vertexBuffer) {
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

    Mesh rv;
    rv.setVerts(vertexBuffer);
    rv.setTexCoords(uvBuffer);
    rv.setIndices(meshIndices);
    if (detail == 0) {
        rv.recalculateNormals();  // flat-shaded
    }
    else {
        auto meshNormals = vertexBuffer;
        for (auto& v : meshNormals) { v = normalize(v); }
        rv.setNormals(meshNormals);
    }
    return rv;
}

Mesh osc::GenerateIcosahedronMesh(
    float radius,
    size_t detail)
{
    float const t = (1.0f + sqrt(5.0f))/2.0f;

    auto const vertices = std::to_array<Vec3>({
        {-1.0f,  t,     0.0f}, {1.0f, t,    0.0f}, {-1.0f, -t,     0.0f}, { 1.0f, -t,     0.0f},
        { 0.0f, -1.0f,  t   }, {0.0f, 1.0f, t   }, { 0.0f, -1.0f, -t   }, { 0.0f,  1.0f, -t   },
        { t,     0.0f, -1.0f}, {t,    0.0f, 1.0f}, {-t,     0.0f, -1.0f}, {-t,     0.0f,  1.0f},
    });

    auto const indices = std::to_array<uint32_t>({
        0, 11, 5,    0, 5,  1,     0,  1,  7,     0,  7, 10,    0, 10, 11,
        1, 5,  9,    5, 11, 4,     11, 10, 2,     10, 7, 6,     7, 1,  8,
        3, 9,  4,    3, 4,  2,     3,  2,  6,     3,  6, 8,     3, 8, 9,
        4, 9,  5,    2, 4,  11,    6,  2,  10,    8,  6, 7,     9, 8, 1,
    });

    return GeneratePolyhedronMesh(vertices, indices, radius, detail);
}

Mesh osc::GenerateDodecahedronMesh(
    float radius,
    size_t detail)
{
    // implementation ported from threejs (DodecahedronGeometry)

    float const t = (1.0f + sqrt(5.0f))/2.0f;
    float const r = 1.0f/t;

    auto const vertices = std::to_array<Vec3>({
        {-1.0f, -1.0f, -1.0f}, {-1.0f, -1.0f, 1.0f},
        {-1.0f,  1.0f, -1.0f}, {-1.0f,  1.0f, 1.0f},
        { 1.0f, -1.0f, -1.0f}, { 1.0f, -1.0f, 1.0f},
        { 1.0f,  1.0f, -1.0f}, { 1.0f,  1.0f, 1.0f},

        { 0.0f, -r, -t}      , {0.0f, -r, t},
        {0, r, - t},           {0, r, t},

        {-r, -t, 0}, {-r, t, 0},
        {r, -t, 0}, {r, t, 0},

        {-t, 0, -r}, {t, 0, -r},
        {-t, 0, r}, {t, 0, r},
    });

    auto const indices = std::to_array<uint32_t>({
        3, 11, 7, 	3, 7, 15, 	3, 15, 13,
        7, 19, 17, 	7, 17, 6, 	7, 6, 15,
        17, 4, 8, 	17, 8, 10, 	17, 10, 6,
        8, 0, 16, 	8, 16, 2, 	8, 2, 10,
        0, 12, 1, 	0, 1, 18, 	0, 18, 16,
        6, 10, 2, 	6, 2, 13, 	6, 13, 15,
        2, 16, 18, 	2, 18, 3, 	2, 3, 13,
        18, 1, 9, 	18, 9, 11, 	18, 11, 3,
        4, 14, 12, 	4, 12, 0, 	4, 0, 8,
        11, 9, 5, 	11, 5, 19, 	11, 19, 7,
        19, 5, 14, 	19, 14, 4, 	19, 4, 17,
        1, 12, 14, 	1, 14, 5, 	1, 5, 9
    });

    return GeneratePolyhedronMesh(vertices, indices, radius, detail);
}

Mesh osc::GenerateOctahedronMesh(
    float radius,
    size_t detail)
{
    // implementation ported from threejs (OctahedronGeometry)

    auto const vertices = std::to_array<Vec3>({
        {1.0f,  0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f,  0.0f},
        {0.0f, -1.0f, 0.0f}, { 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, -1.0f},
    });

    auto const indices = std::to_array<uint32_t>({
        0, 2, 4,    0, 4, 3,    0, 3, 5,
        0, 5, 2,    1, 2, 5,    1, 5, 3,
        1, 3, 4,    1, 4, 2
    });

    return GeneratePolyhedronMesh(vertices, indices, radius, detail);
}

Mesh osc::GenerateTetrahedronMesh(
    float radius,
    size_t detail)
{
    // implementation ported from threejs (TetrahedronGeometry)

    auto const vertices = std::to_array<Vec3>({
        {1, 1, 1}, {-1, -1, 1}, {-1, 1, - 1}, {1, -1, -1},
    });

    auto const indices = std::to_array<uint32_t>({
        2, 1, 0,    0, 3, 2,    1, 3, 0,    2, 3, 1
    });

    return GeneratePolyhedronMesh(vertices, indices, radius, detail);
}

Mesh osc::GenerateLatheMesh(
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

Mesh osc::GenerateCircleMesh(
    float radius,
    size_t segments,
    Radians thetaStart,
    Radians thetaLength)
{
    // this implementation was initially hand-ported from threejs (CircleGeometry)

    segments = max(3_uz, segments);
    auto const fsegments = static_cast<float>(segments);

    std::vector<uint32_t> indices;
    std::vector<Vec3> vertices;
    std::vector<Vec3> normals;
    std::vector<Vec2> uvs;

    // middle vertex
    vertices.emplace_back(0.0f, 0.0f, 0.0f);
    normals.emplace_back(0.0f, 0.0f, 1.0f);
    uvs.emplace_back(0.5f, 0.5f);

    // not-middle vertices
    for (ptrdiff_t s = 0; s <= static_cast<ptrdiff_t>(segments); ++s) {
        auto const fs = static_cast<float>(s);
        auto const segment = thetaStart + (fs/fsegments * thetaLength);
        auto const cosSeg = cos(segment);
        auto const sinSeg = sin(segment);

        vertices.emplace_back(radius * cosSeg, radius * sinSeg, 0.0f);
        normals.emplace_back(0.0f, 0.0f, 1.0f);
        uvs.emplace_back((cosSeg + 1.0f) / 2.0f, (sinSeg + 1.0f) / 2.0f);
    }

    for (uint32_t i = 1; i <= static_cast<uint32_t>(segments); ++i) {
        indices.insert(indices.end(), {i, i+1, 0});
    }

    Mesh rv;
    rv.setVerts(vertices);
    rv.setNormals(normals);
    rv.setTexCoords(uvs);
    rv.setIndices(indices);
    return rv;
}

Mesh osc::GenerateRingMesh(
    float innerRadius,
    float outerRadius,
    size_t thetaSegments,
    size_t phiSegments,
    Radians thetaStart,
    Radians thetaLength)
{
    // this implementation was initially hand-ported from threejs (RingGeometry)

    thetaSegments = max(static_cast<size_t>(3), thetaSegments);
    phiSegments = max(static_cast<size_t>(1), phiSegments);
    auto const fthetaSegments = static_cast<float>(thetaSegments);
    auto const fphiSegments = static_cast<float>(phiSegments);

    std::vector<uint32_t> indices;
    std::vector<Vec3> vertices;
    std::vector<Vec3> normals;
    std::vector<Vec2> uvs;

    float radius = innerRadius;
    float radiusStep = (outerRadius - innerRadius)/fphiSegments;

    // generate vertices, normals, and uvs
    for (size_t j = 0; j <= phiSegments; ++j) {
        for (size_t i = 0; i <= thetaSegments; ++i) {
            auto const fi = static_cast<float>(i);
            Radians segment = thetaStart + (fi/fthetaSegments * thetaLength);

            Vec3 const& v = vertices.emplace_back(radius * cos(segment), radius * sin(segment), 0.0f);
            normals.emplace_back(0.0f, 0.0f, 1.0f);
            uvs.emplace_back(
                (v.x/outerRadius + 1.0f) / 2.0f,
                (v.y/outerRadius + 1.0f) / 2.0f
            );
        }
        radius += radiusStep;
    }

    for (size_t j = 0; j < phiSegments; ++j) {
        size_t const thetaSegmentLevel = j * (thetaSegments + 1);
        for (size_t i = 0; i < thetaSegments; ++i) {
            size_t segment = i + thetaSegmentLevel;

            auto const a = static_cast<uint32_t>(segment);
            auto const b = static_cast<uint32_t>(segment + thetaSegments + 1);
            auto const c = static_cast<uint32_t>(segment + thetaSegments + 2);
            auto const d = static_cast<uint32_t>(segment + 1);

            indices.insert(indices.end(), {a, b, d});
            indices.insert(indices.end(), {b, c, d});
        }
    }

    Mesh rv;
    rv.setVerts(vertices);
    rv.setNormals(normals);
    rv.setTexCoords(uvs);
    rv.setIndices(indices);
    return rv;
}

Mesh osc::GenerateTorusMesh(
    float radius,
    float tube,
    size_t radialSegments,
    size_t tubularSegments,
    Radians arc)
{
    // (ported from three.js/TorusGeometry)

    auto const fradialSegments = static_cast<float>(radialSegments);
    auto const ftubularSegments = static_cast<float>(tubularSegments);

    std::vector<uint32_t> indices;
    std::vector<Vec3>  vertices;
    std::vector<Vec3> normals;
    std::vector<Vec2> uvs;

    for (size_t j = 0; j <= radialSegments; ++j) {
        auto const fj = static_cast<float>(j);
        for (size_t i = 0; i <= tubularSegments; ++i) {
            auto const fi = static_cast<float>(i);
            Radians const u = fi/ftubularSegments * arc;
            Radians const v = fj/fradialSegments * 360_deg;

            Vec3 const& vertex = vertices.emplace_back(
                (radius + tube * cos(v)) * cos(u),
                (radius + tube * cos(v)) * sin(u),
                tube * sin(v)
            );
            normals.push_back(UnitVec3{
                vertex.x - radius*cos(u),
                vertex.y - radius*sin(u),
                vertex.z - 0.0f,
            });
            uvs.emplace_back(fi/ftubularSegments, fj/fradialSegments);
        }
    }

    for (size_t j = 1; j <= radialSegments; ++j) {
        for (size_t i = 1; i <= tubularSegments; ++i) {
            auto const a = static_cast<uint32_t>((tubularSegments + 1)*(j + 0) + i - 1);
            auto const b = static_cast<uint32_t>((tubularSegments + 1)*(j - 1) + i - 1);
            auto const c = static_cast<uint32_t>((tubularSegments + 1)*(j - 1) + i);
            auto const d = static_cast<uint32_t>((tubularSegments + 1)*(j + 0) + i);

            indices.insert(indices.end(), {a, b, d});
            indices.insert(indices.end(), {b, c, d});
        }
    }

    Mesh rv;
    rv.setVerts(vertices);
    rv.setNormals(normals);
    rv.setTexCoords(uvs);
    rv.setIndices(indices);
    return rv;
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
