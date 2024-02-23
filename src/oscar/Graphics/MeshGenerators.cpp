#include "MeshGenerators.h"

#include <oscar/Graphics/Mesh.h>
#include <oscar/Graphics/SubMeshDescriptor.h>
#include <oscar/Maths/Angle.h>
#include <oscar/Maths/CommonFunctions.h>
#include <oscar/Maths/MathHelpers.h>
#include <oscar/Maths/Triangle.h>
#include <oscar/Maths/UnitVec3.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/Shims/Cpp23/cstddef.h>
#include <oscar/Utils/Assertions.h>
#include <oscar/Utils/At.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <numeric>
#include <span>
#include <vector>

using namespace osc::literals;
using namespace osc;

namespace
{
    struct UntexturedVert final {
        Vec3 pos;
        Vec3 norm;
    };

    struct TexturedVert final {
        Vec3 pos;
        Vec3 norm;
        Vec2 uv;
    };

    // standard textured cube with dimensions [-1, +1] in xyz and uv coords of
    // (0, 0) bottom-left, (1, 1) top-right for each (quad) face
    constexpr std::array<TexturedVert, 36> c_ShadedTexturedCubeVerts =
    {{
        // back face
        {{-1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}},  // bottom-left
        {{1.0f, 1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}},  // top-right
        {{1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}},  // bottom-right
        {{1.0f, 1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}},  // top-right
        {{-1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}},  // bottom-left
        {{-1.0f, 1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}},  // top-left

        // front face
        {{-1.0f, -1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},  // bottom-left
        {{1.0f, -1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},  // bottom-right
        {{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},  // top-right
        {{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},  // top-right
        {{-1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},  // top-left
        {{-1.0f, -1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},  // bottom-left

        // left face
        {{-1.0f, 1.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},  // top-right
        {{-1.0f, 1.0f, -1.0f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},  // top-left
        {{-1.0f, -1.0f, -1.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},  // bottom-left
        {{-1.0f, -1.0f, -1.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},  // bottom-left
        {{-1.0f, -1.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},  // bottom-right
        {{-1.0f, 1.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},  // top-right

        // right face
        {{1.0f, 1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},  // top-left
        {{1.0f, -1.0f, -1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},  // bottom-right
        {{1.0f, 1.0f, -1.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},  // top-right
        {{1.0f, -1.0f, -1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},  // bottom-right
        {{1.0f, 1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},  // top-left
        {{1.0f, -1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},  // bottom-left

        // bottom face
        {{-1.0f, -1.0f, -1.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}},  // top-right
        {{1.0f, -1.0f, -1.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f}},  // top-left
        {{1.0f, -1.0f, 1.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}},  // bottom-left
        {{1.0f, -1.0f, 1.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}},  // bottom-left
        {{-1.0f, -1.0f, 1.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}},  // bottom-right
        {{-1.0f, -1.0f, -1.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}},  // top-right

        // top face
        {{-1.0f, 1.0f, -1.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},  // top-left
        {{1.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},  // bottom-right
        {{1.0f, 1.0f, -1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},  // top-right
        {{1.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},  // bottom-right
        {{-1.0f, 1.0f, -1.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},  // top-left
        {{-1.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}}  // bottom-left
    }};

    // standard textured quad
    // - dimensions [-1, +1] in xy and [0, 0] in z
    // - uv coords are (0, 0) bottom-left, (1, 1) top-right
    // - normal is +1 in Z, meaning that it faces toward the camera
    constexpr std::array<TexturedVert, 6> c_ShadedTexturedQuadVerts =
    {{
        // CCW winding (culling)
        {{-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},  // bottom-left
        {{1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},  // bottom-right
        {{1.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},  // top-right

        {{1.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},  // top-right
        {{-1.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},  // top-left
        {{-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},  // bottom-left
    }};

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

Mesh osc::GenerateTexturedQuadMesh()
{
    NewMeshData data;
    data.reserve(c_ShadedTexturedQuadVerts.size());

    uint16_t index = 0;
    for (TexturedVert const& v : c_ShadedTexturedQuadVerts)
    {
        data.verts.push_back(v.pos);
        data.normals.push_back(v.norm);
        data.texcoords.push_back(v.uv);
        data.indices.push_back(index++);
    }

    OSC_ASSERT(data.verts.size() % 3 == 0);
    OSC_ASSERT(data.verts.size() == data.normals.size() && data.verts.size() == data.indices.size());

    return CreateMeshFromData(std::move(data));
}

Mesh osc::GenerateUVSphereMesh(size_t sectors, size_t stacks)
{
    NewMeshData data;
    data.reserve(2LL*3LL*stacks*sectors);

    // this is a shitty alg that produces a shitty UV sphere. I don't have
    // enough time to implement something better, like an isosphere, or
    // something like a patched sphere:
    //
    // https://www.iquilezles.org/www/articles/patchedsphere/patchedsphere.htm
    //
    // This one is adapted from:
    //    http://www.songho.ca/opengl/gl_sphere.html#example_cubesphere


    // polar coords, with [0, 0, -1] pointing towards the screen with polar
    // coords theta = 0, phi = 0. The coordinate [0, 1, 0] is theta = (any)
    // phi = PI/2. The coordinate [1, 0, 0] is theta = PI/2, phi = 0
    std::vector<TexturedVert> points;

    Radians const thetaStep = 360_deg / sectors;
    Radians const phiStep = 180_deg / stacks;

    for (size_t stack = 0; stack <= stacks; ++stack)
    {
        Radians const phi = 90_deg - stack*phiStep;
        float const y = sin(phi);

        for (size_t sector = 0; sector <= sectors; ++sector)
        {
            Radians const theta = sector * thetaStep;
            float const x = sin(theta) * cos(phi);
            float const z = -cos(theta) * cos(phi);
            Vec3 const pos = {x, y, z};
            Vec3 const normal = pos;
            Vec2 const uv =
            {
                static_cast<float>(sector) / static_cast<float>(sectors),
                static_cast<float>(stack) / static_cast<float>(stacks),
            };
            points.push_back({pos, normal, uv});
        }
    }

    // the points are not triangles. They are *points of a triangle*, so the
    // points must be triangulated

    uint16_t index = 0;
    auto push = [&data, &index](TexturedVert const& v)
    {
        data.verts.push_back(v.pos);
        data.normals.push_back(v.norm);
        data.texcoords.push_back(v.uv);
        data.indices.push_back(index++);
    };

    for (size_t stack = 0; stack < stacks; ++stack)
    {
        size_t k1 = stack * (sectors + 1);
        size_t k2 = k1 + sectors + 1;

        for (size_t sector = 0; sector < sectors; ++sector, ++k1, ++k2)
        {
            // 2 triangles per sector - excluding the first and last stacks
            // (which contain one triangle, at the poles)

            TexturedVert const& p1 = points[k1];
            TexturedVert const& p2 = points[k2];
            TexturedVert const& p1Plus1 = points[k1+1];
            TexturedVert const& p2Plus1 = points[k2+1];

            if (stack != 0)
            {
                push(p1);
                push(p1Plus1);
                push(p2);
            }

            if (stack != (stacks - 1))
            {
                push(p1Plus1);
                push(p2Plus1);
                push(p2);
            }
        }
    }

    OSC_ASSERT(data.verts.size() % 3 == 0);
    OSC_ASSERT(data.verts.size() == data.normals.size() && data.verts.size() == data.indices.size());

    return CreateMeshFromData(std::move(data));
}

Mesh osc::GenerateUntexturedYToYCylinderMesh(size_t nsides)
{
    constexpr float c_TopY = +1.0f;
    constexpr float c_BottomY = -1.0f;
    constexpr float c_Radius = 1.0f;
    constexpr float c_TopDirection = c_TopY;
    constexpr float c_BottomDirection = c_BottomY;

    OSC_ASSERT(3 <= nsides && nsides < 1000000 && "the backend only supports 32-bit indices, you should double-check that this code would work (change this assertion if it does)");

    Radians const stepAngle = 360_deg / nsides;

    NewMeshData data;

    // helper: push mesh *data* (i.e. vert and normal) to the output
    auto const pushData = [&data](Vec3 const& pos, Vec3 const& norm)
    {
        auto const idx = static_cast<uint32_t>(data.verts.size());
        data.verts.push_back(pos);
        data.normals.push_back(norm);
        return idx;
    };

    // helper: push primitive *indices* (into data) to the output
    auto const pushTriangle = [&data](uint32_t p0idx, uint32_t p1idx, uint32_t p2idx)
    {
        data.indices.push_back(p0idx);
        data.indices.push_back(p1idx);
        data.indices.push_back(p2idx);
    };

    // top: a triangle fan
    {
        // preemptively push the middle and the first point and hold onto their indices
        // because the middle is used for all triangles in the fan and the first point
        // is used when completing the loop

        Vec3 const topNormal = {0.0f, c_TopDirection, 0.0f};
        uint32_t const midpointIndex = pushData({0.0f, c_TopY, 0.0f}, topNormal);
        uint32_t const loopStartIndex = pushData({c_Radius, c_TopY, 0.0f}, topNormal);

        // then go through each outer vertex one-by-one, creating a triangle between
        // the new vertex, the middle, and the previous vertex

        uint32_t p1Index = loopStartIndex;
        for (size_t side = 1; side < nsides; ++side)
        {
            Radians const theta = side * stepAngle;
            Vec3 const p2 = {c_Radius*cos(theta), c_TopY, c_Radius*sin(theta)};
            uint32_t const p2Index = pushData(p2, topNormal);

            // care: the outer-facing direction must wind counter-clockwise (#626)
            pushTriangle(midpointIndex, p2Index, p1Index);
            p1Index = p2Index;
        }

        // finish loop
        pushTriangle(midpointIndex, loopStartIndex, p1Index);
    }

    // bottom: another triangle fan
    {
        // preemptively push the middle and the first point and hold onto their indices
        // because the middle is used for all triangles in the fan and the first point
        // is used when completing the loop

        Vec3 const bottomNormal = {0.0f, c_BottomDirection, 0.0f};
        uint32_t const midpointIndex = pushData({0.0f, c_BottomY, 0.0f}, bottomNormal);
        uint32_t const loopStartIndex = pushData({c_Radius, c_BottomY, 0.0f}, bottomNormal);

        // then go through each outer vertex one-by-one, creating a triangle between
        // the new vertex, the middle, and the previous vertex

        uint32_t p1Index = loopStartIndex;
        for (size_t side = 1; side < nsides; ++side)
        {
            Radians const theta = side * stepAngle;
            Vec3 const p2 = {c_Radius*cos(theta), c_BottomY, c_Radius*sin(theta)};
            uint32_t const p2Index = pushData(p2, bottomNormal);

            // care: the outer-facing direction must wind counter-clockwise (#626)
            pushTriangle(midpointIndex, p1Index, p2Index);
            p1Index = p2Index;
        }

        // finish loop
        pushTriangle(midpointIndex, p1Index, loopStartIndex);
    }

    // sides: a loop of quads along the edges
    constexpr bool smoothShaded = true;
    static_assert(smoothShaded, "if you want rigid edges then it requires copying edge loops etc");

    if (smoothShaded)
    {
        Vec3 const initialNormal = Vec3{1.0f, 0.0f, 0.0f};
        uint32_t const firstEdgeTop = pushData({c_Radius, c_TopY, 0.0f}, initialNormal);
        uint32_t const firstEdgeBottom = pushData({c_Radius, c_BottomY, 0.0f}, initialNormal);

        uint32_t e1TopIdx = firstEdgeTop;
        uint32_t e1BottomIdx = firstEdgeBottom;
        for (size_t i = 1; i < nsides; ++i)
        {
            Radians const theta = i * stepAngle;
            float const xDir = cos(theta);
            float const zDir = sin(theta);
            float const x = c_Radius * xDir;
            float const z = c_Radius * zDir;

            Vec3 const normal = {xDir, 0.0f, zDir};
            uint32_t const e2TopIdx = pushData({x, c_TopY, z}, normal);
            uint32_t const e2BottomIdx = pushData({x, c_BottomY, z}, normal);

            // care: the outer-facing direction must wind counter-clockwise (#626)
            pushTriangle(e1TopIdx, e2TopIdx, e1BottomIdx);
            pushTriangle(e2TopIdx, e2BottomIdx, e1BottomIdx);

            e1TopIdx = e2TopIdx;
            e1BottomIdx = e2BottomIdx;
        }
        // finish loop (making sure to wind it correctly - #626)
        pushTriangle(e1TopIdx, firstEdgeTop, e1BottomIdx);
        pushTriangle(firstEdgeTop, firstEdgeBottom, e1BottomIdx);
    }

    return CreateMeshFromData(std::move(data));
}

Mesh osc::GenerateUntexturedYToYConeMesh(size_t nsides)
{
    NewMeshData data;
    data.reserve(static_cast<size_t>(2*3)*nsides);

    constexpr float topY = +1.0f;
    constexpr float bottomY = -1.0f;
    Radians const stepAngle = 360_deg / nsides;

    uint16_t index = 0;
    auto const push = [&data, &index](Vec3 const& pos, Vec3 const& norm)
    {
        data.verts.push_back(pos);
        data.normals.push_back(norm);
        data.indices.push_back(index++);
    };

    // bottom
    {
        Vec3 const normal = {0.0f, -1.0f, 0.0f};
        Vec3 const middle = {0.0f, bottomY, 0.0f};

        for (size_t i = 0; i < nsides; ++i)
        {
            Radians const thetaStart = i * stepAngle;
            Radians const thetaEnd = (i + 1) * stepAngle;

            Vec3 const p1 = {cos(thetaStart), bottomY, sin(thetaStart)};
            Vec3 const p2 = {cos(thetaEnd), bottomY, sin(thetaEnd)};

            push(middle, normal);
            push(p1, normal);
            push(p2, normal);
        }
    }

    // sides
    {
        for (size_t i = 0; i < nsides; ++i)
        {
            Radians const thetaStart = i * stepAngle;
            Radians const thetaEnd = (i + 1) * stepAngle;

            Triangle const triangle =
            {
                {0.0f, topY, 0.0f},
                {cos(thetaEnd), bottomY, sin(thetaEnd)},
                {cos(thetaStart), bottomY, sin(thetaStart)},
            };

            Vec3 const normal = TriangleNormal(triangle);

            push(triangle.p0, normal);
            push(triangle.p1, normal);
            push(triangle.p2, normal);
        }
    }

    OSC_ASSERT(data.verts.size() % 3 == 0);
    OSC_ASSERT(data.verts.size() == data.normals.size() && data.verts.size() == data.indices.size());

    return CreateMeshFromData(std::move(data));
}

Mesh osc::GenerateNbyNGridLinesMesh(size_t n)
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

Mesh osc::GenerateCubeMesh()
{
    NewMeshData data;
    data.reserve(c_ShadedTexturedCubeVerts.size());

    uint16_t index = 0;
    for (TexturedVert const& v : c_ShadedTexturedCubeVerts)
    {
        data.verts.push_back(v.pos);
        data.normals.push_back(v.norm);
        data.texcoords.push_back(v.uv);
        data.indices.push_back(index++);
    }

    OSC_ASSERT(data.verts.size() % 3 == 0);
    OSC_ASSERT(data.verts.size() == data.normals.size() && data.verts.size() == data.indices.size());

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

Mesh osc::GenerateCircleMesh(size_t nsides)
{
    NewMeshData data;
    data.verts.reserve(3*nsides);
    data.topology = MeshTopology::Triangles;

    uint16_t index = 0;
    auto push = [&data, &index](float x, float y, float z)
    {
        data.verts.emplace_back(x, y, z);
        data.indices.push_back(index++);
        data.normals.emplace_back(0.0f, 0.0f, 1.0f);
    };

    Radians const step = 360_deg / nsides;
    for (size_t i = 0; i < nsides; ++i)
    {
        Radians const theta1 = static_cast<float>(i) * step;
        Radians const theta2 = static_cast<float>(i + 1) * step;

        push(0.0f, 0.0f, 0.0f);
        push(sin(theta1), cos(theta1), 0.0f);
        push(sin(theta2), cos(theta2), 0.0f);
    }

    return CreateMeshFromData(std::move(data));
}

Mesh osc::GenerateTorusMesh(size_t slices, size_t stacks, float torusCenterToTubeCenterRadius, float tubeRadius)
{
    // adapted from GitHub:prideout/par (used by raylib internally)

    if (slices < 3 || stacks < 3)
    {
        return Mesh{};
    }

    auto const torusFn = [torusCenterToTubeCenterRadius, tubeRadius](Vec2 const uv)
    {
        Radians const theta = 360_deg * uv.x;
        Radians const phi = 360_deg * uv.y;
        float const beta = torusCenterToTubeCenterRadius + tubeRadius*cos(phi);

        return Vec3
        {
            cos(theta) * beta,
            sin(theta) * beta,
            sin(phi) * tubeRadius,
        };
    };

    NewMeshData data;
    data.verts.reserve((slices+1) * (stacks+1));
    data.texcoords.reserve(data.verts.size());
    data.normals.reserve(data.verts.size());
    data.indices.reserve(2 * slices * stacks);

    // generate verts+texcoords
    for (size_t stack = 0; stack < stacks+1; ++stack)
    {
        for (size_t slice = 0; slice < slices+1; ++slice)
        {
            Vec2 const uv =
            {
                static_cast<float>(stack)/static_cast<float>(stacks),
                static_cast<float>(slice)/static_cast<float>(slices),
            };
            data.texcoords.push_back(uv);
            data.verts.push_back(torusFn(uv));
        }
    }

    // generate faces
    {
        auto const safePush = [&data](size_t index)
        {
            OSC_ASSERT(index < data.verts.size());
            data.indices.push_back(static_cast<uint32_t>(index));
        };

        size_t v = 0;
        for (size_t stack = 0; stack < stacks; ++stack)
        {
            for (size_t slice = 0; slice < slices; ++slice)
            {
                size_t const next = slice + 1;
                safePush(v + slice + slices + 1);
                safePush(v + next);
                safePush(v + slice);
                safePush(v + slice + slices + 1);
                safePush(v + next + slices + 1);
                safePush(v + next);
            }
            v += slices + 1;
        }
    }

    // generate normals from faces
    {
        OSC_ASSERT(data.indices.size() % 3 == 0);
        data.normals.resize(data.verts.size());
        for (size_t i = 0; i+2 < data.indices.size(); i += 3)
        {
            Triangle const t =
            {
                data.verts.at(data.indices[i]),
                data.verts.at(data.indices[i+1]),
                data.verts.at(data.indices[i+2]),
            };
            Triangle const t1 =
            {
                data.verts.at(data.indices[i+1]),
                data.verts.at(data.indices[i+2]),
                data.verts.at(data.indices[i]),
            };
            Triangle const t2 =
            {
                data.verts.at(data.indices[i+2]),
                data.verts.at(data.indices[i]),
                data.verts.at(data.indices[i+1]),
            };

            data.normals.at(data.indices[i]) = TriangleNormal(t);
            data.normals.at(data.indices[i+1]) = TriangleNormal(t1);
            data.normals.at(data.indices[i+2]) = TriangleNormal(t2);
        }
        OSC_ASSERT(data.normals.size() == data.verts.size());
    }

    return CreateMeshFromData(std::move(data));
}

Mesh osc::GenerateNxMGridLinesMesh(Vec2 min, Vec2 max, Vec2i steps)
{
    // all Z values in the returned mesh shall be 0
    constexpr float zValue = 0.0f;

    if (steps.x <= 0 || steps.y <= 0)
    {
        // edge case: no steps specified: return empty mesh
        return {};
    }

    // ensure the indices can fit the requested grid
    {
        OSC_ASSERT(steps.x*steps.y <= std::numeric_limits<int32_t>::max() && "requested a grid size that is too large for the mesh class");
    }

    // create vector of grid points
    std::vector<Vec3> verts;
    verts.reserve(static_cast<size_t>(steps.x) * static_cast<size_t>(steps.y));

    // create vector of line indices (indices to the two points that make a grid line)
    std::vector<uint32_t> indices;
    indices.reserve(static_cast<size_t>(4) * static_cast<size_t>(steps.x) * static_cast<size_t>(steps.y));

    // precompute spatial step between points
    Vec2 const stepSize = (max - min) / Vec2{steps - 1};

    // push first row (no verticals)
    {
        // emit top-leftmost point (no links)
        {
            verts.emplace_back(min, zValue);
        }

        // emit rest of the first row (only has horizontal links)
        for (int32_t x = 1; x < steps.x; ++x)
        {
            Vec3 const pos = {min.x + static_cast<float>(x)*stepSize.x, min.y, zValue};
            verts.push_back(pos);
            uint32_t const index = static_cast<int32_t>(verts.size() - 1);
            indices.push_back(index - 1);  // link to previous point
            indices.push_back(index);      // and then the new point
        }

        OSC_ASSERT(verts.size() == static_cast<size_t>(steps.x) && "all points in the first row have not been emitted");
        OSC_ASSERT(indices.size() == static_cast<size_t>(2 * (steps.x - 1)) && "all lines in the first row have not been emitted");
    }

    // push remaining rows (all points have verticals, first point of each row has no horizontal)
    for (int32_t y = 1; y < steps.y; ++y)
    {
        // emit leftmost point (only has a vertical link)
        {
            verts.emplace_back(min.x, min.y + static_cast<float>(y)*stepSize.y, zValue);
            uint32_t const index = static_cast<int32_t>(verts.size() - 1);
            indices.push_back(index - steps.x);  // link the point one row above
            indices.push_back(index);            // to the point (vertically)
        }

        // emit rest of the row (has vertical and horizontal links)
        for (int32_t x = 1; x < steps.x; ++x)
        {
            Vec3 const pos = {min.x + static_cast<float>(x)*stepSize.x, min.y + static_cast<float>(y)*stepSize.y, zValue};
            verts.push_back(pos);
            uint32_t const index = static_cast<int32_t>(verts.size() - 1);
            indices.push_back(index - 1);        // link the previous point
            indices.push_back(index);            // to the current point (horizontally)
            indices.push_back(index - steps.x);  // link the point one row above
            indices.push_back(index);            // to the current point (vertically)
        }
    }

    OSC_ASSERT(verts.size() == static_cast<size_t>(steps.x*steps.y) && "incorrect number of vertices emitted");
    OSC_ASSERT(indices.size() <= static_cast<size_t>(4 * steps.y * steps.y) && "too many indices were emitted?");

    // emit data as a renderable mesh
    Mesh rv;
    rv.setTopology(MeshTopology::Lines);
    rv.setVerts(verts);
    rv.setIndices(indices);
    return rv;
}

Mesh osc::GenerateNxMTriangleQuadGridMesh(Vec2i steps)
{
    // all Z values in the returned mesh shall be 0
    constexpr float zValue = 0.0f;

    if (steps.x <= 0 || steps.y <= 0)
    {
        // edge case: no steps specified: return empty mesh
        return {};
    }

    // ensure the indices can fit the requested grid
    {
        OSC_ASSERT(steps.x*steps.y <= std::numeric_limits<int32_t>::max() && "requested a grid size that is too large for the mesh class");
    }

    // create a vector of triangle verts
    std::vector<Vec3> verts;
    verts.reserve(Area(steps));

    // create a vector of texture coordinates (1:1 with verts)
    std::vector<Vec2> coords;
    coords.reserve(Area(steps));

    // create a vector of triangle primitive indices (2 triangles, or 6 indices, per grid cell)
    std::vector<uint32_t> indices;
    indices.reserve(static_cast<size_t>(6) * Area(steps-1));

    // precompute step/min in each direction
    Vec2 const vectorStep = Vec2{2.0f, 2.0f} / Vec2{steps - 1};
    Vec2 const uvStep = Vec2{1.0f, 1.0f} / Vec2{steps - 1};
    Vec2 const vectorMin = {-1.0f, -1.0f};
    Vec2 const uvMin = {0.0f, 0.0f};

    // push first row of verts + texture coords for all columns
    for (int32_t col = 0; col < steps.x; ++col)
    {
        verts.emplace_back(vectorMin.x + static_cast<float>(col)*vectorStep.x, vectorMin.y, zValue);
        coords.emplace_back(uvMin.x + static_cast<float>(col)*uvStep.x, uvMin.y);
    }

    // then work through the next rows, which can safely assume there's a row above them
    for (int32_t row = 1; row < steps.y; ++row)
    {
        auto const rowf = static_cast<float>(row);

        // push point + coord of the first column's left-edge
        verts.emplace_back(vectorMin.x, vectorMin.y + rowf*vectorStep.y, zValue);
        coords.emplace_back(uvMin.x, uvMin.y + rowf*uvStep.y);

        // then, for all remaining columns, push the right-edge data and the triangles
        for (int32_t col = 1; col < steps.x; ++col)
        {
            auto const colf = static_cast<float>(col);
            verts.emplace_back(vectorMin.x + colf*vectorStep.x, vectorMin.y + rowf*vectorStep.y, zValue);
            coords.emplace_back(uvMin.x + colf*uvStep.x, uvMin.y + rowf*uvStep.y);

            // triangles (anti-clockwise wound)
            int32_t const currentIdx = row*steps.x + col;
            int32_t const bottomRightIdx = currentIdx;
            int32_t const bottomLeftIdx = currentIdx - 1;
            int32_t const topLeftIdx =  bottomLeftIdx - steps.x;
            int32_t const topRightIdx = bottomRightIdx - steps.x;

            // top-left triangle
            indices.push_back(topRightIdx);
            indices.push_back(topLeftIdx);
            indices.push_back(bottomLeftIdx);

            // bottom-right triangle
            indices.push_back(topRightIdx);
            indices.push_back(bottomLeftIdx);
            indices.push_back(bottomRightIdx);
        }
    }

    OSC_ASSERT(verts.size() == coords.size());
    OSC_ASSERT(std::ssize(indices) == static_cast<ptrdiff_t>((steps.x-1)*(steps.y-1)*6));

    Mesh rv;
    rv.setTopology(MeshTopology::Triangles);
    rv.setVerts(verts);
    rv.setTexCoords(coords);
    rv.setIndices(indices);
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
            Vec3 const aj = mix(a, c, fi/fcols);
            Vec3 const bj = mix(b, c, fi/fcols);

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
                    v.at(i).at(j) = mix(aj, bj, fj/frows);
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
        for (size_t i = 0; i < indices.size(); i += 3) {
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

        for (size_t i = 0; i < vertexBuffer.size(); i += 3) {
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
        for (size_t i = 0; i < uvBuffer.size(); i += 3) {
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

    auto meshNormals = vertexBuffer;
    for (auto& v : meshNormals) { v = normalize(v); }

    // TODO
    // if (detail == 0) {
    //     compute flat normals: (three.js: computeVertexNormals)
    // }
    // else {
    //     compute smooth normals (three.js: normalizeNormals)
    // }

    Mesh rv;
    rv.setVerts(vertexBuffer);
    rv.setNormals(meshNormals);
    rv.setTexCoords(uvBuffer);
    rv.setIndices(meshIndices);
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

    float const fsegments = static_cast<float>(segments);
    float const inverseSegments = 1.0f/fsegments;
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

Mesh osc::GenerateCircleMesh2(
    float radius,
    size_t segments,
    Radians thetaStart,
    Radians thetaLength)
{
    // this implementation was initially hand-ported from threejs (CircleGeometry)

    segments = max(static_cast<size_t>(3), segments);
    float const fsegments = static_cast<float>(segments);

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
        float const fs = static_cast<float>(s);
        Radians const segment = thetaStart + (fs/fsegments * thetaLength);

        vertices.emplace_back(radius * cos(segment), radius * sin(segment), 0.0f);
        normals.emplace_back(0.0f, 0.0f, 1.0f);

        uvs.emplace_back(
            (vertices[s-1].x/radius + 1.0f) / 2.0f,
            (vertices[s-1].y/radius + 1.0f) / 2.0f
        );
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
    float const fthetaSegments = static_cast<float>(thetaSegments);
    float const fphiSegments = static_cast<float>(phiSegments);

    std::vector<uint32_t> indices;
    std::vector<Vec3> vertices;
    std::vector<Vec3> normals;
    std::vector<Vec2> uvs;

    float radius = innerRadius;
    float radiusStep = (outerRadius - innerRadius)/fphiSegments;

    // generate vertices, normals, and uvs
    for (size_t j = 0; j <= phiSegments; ++j) {
        for (size_t i = 0; i <= thetaSegments; ++i) {
            float const fi = static_cast<float>(i);
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

Mesh osc::GenerateTorusMesh2(
    float radius,
    float tube,
    size_t radialSegments,
    size_t tubularSegments,
    Radians arc)
{
    // (ported from three.js/TorusGeometry)

    float const fradialSegments = static_cast<float>(radialSegments);
    float const ftubularSegments = static_cast<float>(tubularSegments);

    std::vector<uint32_t> indices;
    std::vector<Vec3>  vertices;
    std::vector<Vec3> normals;
    std::vector<Vec2> uvs;

    for (size_t j = 0; j <= radialSegments; ++j) {
        float const fj = static_cast<float>(j);
        for (size_t i = 0; i <= tubularSegments; ++i) {
            float const fi = static_cast<float>(i);
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

Mesh osc::GenerateCylinderMesh2(
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
                float const fx = static_cast<float>(x);
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
            uint32_t const c = static_cast<uint32_t>(centerIndexStart + x);
            uint32_t const i = static_cast<uint32_t>(centerIndexEnd + x);

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

Mesh osc::GenerateConeMesh2(
    float radius,
    float height,
    size_t radialSegments,
    size_t heightSegments,
    bool openEnded,
    Radians thetaStart,
    Radians thetaLength)
{
    return GenerateCylinderMesh2(
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

Mesh osc::GeneratePlaneMesh2(
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

Mesh osc::GenerateSphereMesh2(
    float radius,
    size_t widthSegments,
    size_t heightSegments,
    Radians phiStart,
    Radians phiLength,
    Radians thetaStart,
    Radians thetaLength)
{
    // implementation was initially hand-ported from three.js (SphereGeometry)

    widthSegments = max(3_zu, widthSegments);
    heightSegments = max(2_zu, heightSegments);
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
