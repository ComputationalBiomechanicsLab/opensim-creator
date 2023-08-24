#include "MeshGen.hpp"

#include "oscar/Graphics/Mesh.hpp"
#include "oscar/Maths/Constants.hpp"
#include "oscar/Maths/MathHelpers.hpp"
#include "oscar/Maths/Triangle.hpp"
#include "oscar/Utils/Assertions.hpp"
#include "oscar/Utils/Cpp20Shims.hpp"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace
{
    struct UntexturedVert final {
        glm::vec3 pos;
        glm::vec3 norm;
    };

    struct TexturedVert final {
        glm::vec3 pos;
        glm::vec3 norm;
        glm::vec2 uv;
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

    // a cube wire mesh, suitable for `osc::MeshTopology::Lines` drawing
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
            topology = osc::MeshTopology::Triangles;
        }

        void reserve(size_t s)
        {
            verts.reserve(s);
            normals.reserve(s);
            texcoords.reserve(s);
            indices.reserve(s);
        }

        std::vector<glm::vec3> verts;
        std::vector<glm::vec3> normals;
        std::vector<glm::vec2> texcoords;
        std::vector<uint32_t> indices;
        osc::MeshTopology topology = osc::MeshTopology::Triangles;
    };

    osc::Mesh CreateMeshFromData(NewMeshData&& data)
    {
        osc::Mesh rv;
        rv.setTopology(data.topology);
        rv.setVerts(data.verts);
        rv.setNormals(data.normals);
        rv.setTexCoords(data.texcoords);
        rv.setIndices(data.indices);
        return rv;
    }
}

osc::Mesh osc::GenTexturedQuad()
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

osc::Mesh osc::GenSphere(size_t sectors, size_t stacks)
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

    float const thetaStep = 2.0f * fpi / static_cast<float>(sectors);
    float const phiStep = fpi / static_cast<float>(stacks);

    for (size_t stack = 0; stack <= stacks; ++stack)
    {
        float const phi = fpi2 - static_cast<float>(stack) * phiStep;
        float const y = std::sin(phi);

        for (size_t sector = 0; sector <= sectors; ++sector)
        {
            float const theta = static_cast<float>(sector) * thetaStep;
            float const x = std::sin(theta) * std::cos(phi);
            float const z = -std::cos(theta) * std::cos(phi);
            glm::vec3 const pos = {x, y, z};
            glm::vec3 const normal = pos;
            glm::vec2 const uv =
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

osc::Mesh osc::GenUntexturedYToYCylinder(size_t nsides)
{
    constexpr float c_TopY = +1.0f;
    constexpr float c_BottomY = -1.0f;
    constexpr float c_Radius = 1.0f;
    constexpr float c_TopDirection = c_TopY;
    constexpr float c_BottomDirection = c_BottomY;

    OSC_ASSERT(3 <= nsides && nsides < 1000000 && "the backend only supports 32-bit indices, you should double-check that this code would work (change this assertion if it does)");

    float const stepAngle = 2.0f*fpi / static_cast<float>(nsides);

    NewMeshData data;

    // helper: push mesh *data* (i.e. vert and normal) to the output
    auto const pushData = [&data](glm::vec3 const& pos, glm::vec3 const& norm)
    {
        auto const idx = static_cast<uint32_t>(data.verts.size());
        data.verts.push_back(pos);
        data.normals.push_back(norm);
        return idx;
    };

    // helper: push primitive *indices* (into data) to the output
    auto const pushTriangle = [&data](uint32_t p0Index, uint32_t p1Index, uint32_t p2Index)
    {
        data.indices.push_back(p0Index);
        data.indices.push_back(p1Index);
        data.indices.push_back(p2Index);
    };

    // top: a triangle fan
    {
        // preemptively push the middle and the first point and hold onto their indices
        // because the middle is used for all triangles in the fan and the first point
        // is used when completing the loop

        glm::vec3 const topNormal = {0.0f, c_TopDirection, 0.0f};
        uint32_t const midpointIndex = pushData({0.0f, c_TopY, 0.0f}, topNormal);
        uint32_t const loopStartIndex = pushData({c_Radius, c_TopY, 0.0f}, topNormal);

        // then go through each outer vertex one-by-one, creating a triangle between
        // the new vertex, the middle, and the previous vertex

        uint32_t p1Index = loopStartIndex;
        for (size_t side = 1; side < nsides; ++side)
        {
            float const theta = static_cast<float>(side) * stepAngle;
            glm::vec3 const p2 = {c_Radius*std::cos(theta), c_TopY, c_Radius*std::sin(theta)};
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

        glm::vec3 const bottomNormal = {0.0f, c_BottomDirection, 0.0f};
        uint32_t const midpointIndex = pushData({0.0f, c_BottomY, 0.0f}, bottomNormal);
        uint32_t const loopStartIndex = pushData({c_Radius, c_BottomY, 0.0f}, bottomNormal);

        // then go through each outer vertex one-by-one, creating a triangle between
        // the new vertex, the middle, and the previous vertex

        uint32_t p1Index = loopStartIndex;
        for (size_t side = 1; side < nsides; ++side)
        {
            float const theta = static_cast<float>(side) * stepAngle;
            glm::vec3 const p2 = {c_Radius*std::cos(theta), c_BottomY, c_Radius*std::sin(theta)};
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
        glm::vec3 const initialNormal = glm::vec3{1.0f, 0.0f, 0.0f};
        uint32_t const firstEdgeTop = pushData({c_Radius, c_TopY, 0.0f}, initialNormal);
        uint32_t const firstEdgeBottom = pushData({c_Radius, c_BottomY, 0.0f}, initialNormal);

        uint32_t e1TopIdx = firstEdgeTop;
        uint32_t e1BottomIdx = firstEdgeBottom;
        for (size_t i = 1; i < nsides; ++i)
        {
            float const theta = static_cast<float>(i) * stepAngle;
            float const xDir = std::cos(theta);
            float const zDir = std::sin(theta);
            float const x = c_Radius * xDir;
            float const z = c_Radius * zDir;

            glm::vec3 const normal = {xDir, 0.0f, zDir};
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

osc::Mesh osc::GenUntexturedYToYCone(size_t nsides)
{
    NewMeshData data;
    data.reserve(static_cast<size_t>(2*3)*nsides);

    constexpr float topY = +1.0f;
    constexpr float bottomY = -1.0f;
    const float stepAngle = 2.0f*fpi / static_cast<float>(nsides);

    uint16_t index = 0;
    auto const push = [&data, &index](glm::vec3 const& pos, glm::vec3 const& norm)
    {
        data.verts.push_back(pos);
        data.normals.push_back(norm);
        data.indices.push_back(index++);
    };

    // bottom
    {
        glm::vec3 const normal = {0.0f, -1.0f, 0.0f};
        glm::vec3 const middle = {0.0f, bottomY, 0.0f};

        for (size_t i = 0; i < nsides; ++i)
        {
            float const thetaStart = static_cast<float>(i) * stepAngle;
            float const thetaEnd = static_cast<float>(i + 1) * stepAngle;

            glm::vec3 const p1 = {std::cos(thetaStart), bottomY, std::sin(thetaStart)};
            glm::vec3 const p2 = {std::cos(thetaEnd), bottomY, std::sin(thetaEnd)};

            push(middle, normal);
            push(p1, normal);
            push(p2, normal);
        }
    }

    // sides
    {
        for (size_t i = 0; i < nsides; ++i)
        {
            float const thetaStart = static_cast<float>(i) * stepAngle;
            float const thetaEnd = static_cast<float>(i + 1) * stepAngle;

            Triangle const triangle =
            {
                {0.0f, topY, 0.0f},
                {std::cos(thetaEnd), bottomY, std::sin(thetaEnd)},
                {std::cos(thetaStart), bottomY, std::sin(thetaStart)},
            };

            glm::vec3 const normal = osc::TriangleNormal(triangle);

            push(triangle.p0, normal);
            push(triangle.p1, normal);
            push(triangle.p2, normal);
        }
    }

    OSC_ASSERT(data.verts.size() % 3 == 0);
    OSC_ASSERT(data.verts.size() == data.normals.size() && data.verts.size() == data.indices.size());

    return CreateMeshFromData(std::move(data));
}

osc::Mesh osc::GenNbyNGrid(size_t n)
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
    auto push = [&index, &data](glm::vec3 const& pos)
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

osc::Mesh osc::GenYLine()
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

osc::Mesh osc::GenCube()
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

osc::Mesh osc::GenCubeLines()
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

osc::Mesh osc::GenCircle(size_t nsides)
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

    float const step = 2.0f*fpi / static_cast<float>(nsides);
    for (size_t i = 0; i < nsides; ++i)
    {
        float const theta1 = static_cast<float>(i) * step;
        float const theta2 = static_cast<float>(i + 1) * step;

        push(0.0f, 0.0f, 0.0f);
        push(std::sin(theta1), std::cos(theta1), 0.0f);
        push(std::sin(theta2), std::cos(theta2), 0.0f);
    }

    return CreateMeshFromData(std::move(data));
}

osc::Mesh osc::GenLearnOpenGLCube()
{
    osc::Mesh cube = osc::GenCube();

    cube.transformVerts([](nonstd::span<glm::vec3> vs)
    {
        for (glm::vec3& v : vs)
        {
            v *= 0.5f;
        }
    });

    return cube;
}

osc::Mesh osc::GenTorus(size_t slices, size_t stacks, float torusCenterToTubeCenterRadius, float tubeRadius)
{
    // adapted from GitHub:prideout/par (used by raylib internally)

    if (slices < 3 || stacks < 3)
    {
        return osc::Mesh{};
    }

    auto const torusFn = [torusCenterToTubeCenterRadius, tubeRadius](glm::vec2 const uv)
    {
        float const theta = 2.0f * fpi * uv.x;
        float const phi = 2.0f * fpi * uv.y;
        float const beta = torusCenterToTubeCenterRadius + tubeRadius*std::cos(phi);

        return glm::vec3
        {
            std::cos(theta) * beta,
            std::sin(theta) * beta,
            std::sin(phi) * tubeRadius,
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
            glm::vec2 const uv =
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
            osc::Triangle const t =
            {
                data.verts.at(data.indices[i]),
                data.verts.at(data.indices[i+1]),
                data.verts.at(data.indices[i+2]),
            };
            osc::Triangle const t1 =
            {
                data.verts.at(data.indices[i+1]),
                data.verts.at(data.indices[i+2]),
                data.verts.at(data.indices[i]),
            };
            osc::Triangle const t2 =
            {
                data.verts.at(data.indices[i+2]),
                data.verts.at(data.indices[i]),
                data.verts.at(data.indices[i+1]),
            };

            data.normals.at(data.indices[i]) = osc::TriangleNormal(t);
            data.normals.at(data.indices[i+1]) = osc::TriangleNormal(t1);
            data.normals.at(data.indices[i+2]) = osc::TriangleNormal(t2);
        }
        OSC_ASSERT(data.normals.size() == data.verts.size());
    }

    return CreateMeshFromData(std::move(data));
}

osc::Mesh osc::GenNxMPoint2DGridWithConnectingLines(glm::vec2 min, glm::vec2 max, glm::ivec2 steps)
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
    std::vector<glm::vec3> verts;
    verts.reserve(static_cast<size_t>(steps.x) * static_cast<size_t>(steps.y));

    // create vector of line indices (indices to the two points that make a grid line)
    std::vector<uint32_t> indices;
    indices.reserve(static_cast<size_t>(4) * static_cast<size_t>(steps.x) * static_cast<size_t>(steps.y));

    // precompute spatial step between points
    glm::vec2 const stepSize = (max - min) / glm::vec2{steps - 1};

    // push first row (no verticals)
    {
        // emit top-leftmost point (no links)
        {
            verts.emplace_back(min, zValue);
        }

        // emit rest of the first row (only has horizontal links)
        for (int32_t x = 1; x < steps.x; ++x)
        {
            glm::vec3 const pos = {min.x + static_cast<float>(x)*stepSize.x, min.y, zValue};
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
            glm::vec3 const pos = {min.x + static_cast<float>(x)*stepSize.x, min.y + static_cast<float>(y)*stepSize.y, zValue};
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
    osc::Mesh rv;
    rv.setTopology(osc::MeshTopology::Lines);
    rv.setVerts(verts);
    rv.setIndices(indices);
    return rv;
}

osc::Mesh osc::GenNxMTriangleQuad2DGrid(glm::ivec2 steps)
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
    std::vector<glm::vec3> verts;
    verts.reserve(static_cast<size_t>(steps.x) * static_cast<size_t>(steps.y));

    // create a vector of texture coordinates (1:1 with verts)
    std::vector<glm::vec2> coords;
    coords.reserve(static_cast<size_t>(steps.x) * static_cast<size_t>(steps.y));

    // create a vector of triangle primitive indices (2 triangles, or 6 indices, per grid cell)
    std::vector<uint32_t> indices;
    indices.reserve(static_cast<size_t>(6) * static_cast<size_t>(steps.x-1) * static_cast<size_t>(steps.y-1));

    // precompute step/min in each direction
    glm::vec2 const vectorStep = glm::vec2{2.0f, 2.0f} / glm::vec2{steps - 1};
    glm::vec2 const uvStep = glm::vec2{1.0f, 1.0f} / glm::vec2{steps - 1};
    glm::vec2 const vectorMin = {-1.0f, -1.0f};
    glm::vec2 const uvMin = {0.0f, 0.0f};

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
    OSC_ASSERT(ssize(indices) == static_cast<ptrdiff_t>((steps.x-1)*(steps.y-1)*6));

    osc::Mesh rv;
    rv.setTopology(osc::MeshTopology::Triangles);
    rv.setVerts(verts);
    rv.setTexCoords(coords);
    rv.setIndices(indices);
    return rv;
}
