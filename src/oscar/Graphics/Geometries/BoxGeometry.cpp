#include "BoxGeometry.h"

#include <oscar/Graphics/Mesh.h>
#include <oscar/Graphics/MeshTopology.h>
#include <oscar/Graphics/SubMeshDescriptor.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/Maths/Vec3.h>

#include <cstddef>
#include <cstdint>
#include <vector>

using namespace osc;

osc::BoxGeometry::BoxGeometry(
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
    const auto buildPlane = [&indices, &vertices, &normals, &uvs, &submeshes, &numberOfVertices, &groupStart](
        Vec3::size_type u,
        Vec3::size_type v,
        Vec3::size_type w,
        float udir,
        float vdir,
        Vec3 dims,
        size_t gridX,
        size_t gridY)
    {
        const float segmentWidth = dims.x / static_cast<float>(gridX);
        const float segmentHeight = dims.y / static_cast<float>(gridY);

        const float widthHalf = 0.5f * dims.x;
        const float heightHalf = 0.5f * dims.y;
        const float depthHalf = 0.5f * dims.z;

        const size_t gridX1 = gridX + 1;
        const size_t gridY1 = gridY + 1;

        size_t vertexCount = 0;
        size_t groupCount = 0;

        // generate vertices, normals, and UVs
        for (size_t iy = 0; iy < gridY1; ++iy) {
            const float y = static_cast<float>(iy)*segmentHeight - heightHalf;
            for (size_t ix = 0; ix < gridX1; ++ix) {
                const float x = static_cast<float>(ix)*segmentWidth - widthHalf;

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

                uvs.emplace_back(static_cast<float>(ix)/static_cast<float>(gridX), 1.0f - static_cast<float>(iy)/static_cast<float>(gridY));

                ++vertexCount;
            }
        }

        // indices (two triangles, or 6 indices, per segment)
        for (size_t iy = 0; iy < gridY; ++iy) {
            for (size_t ix = 0; ix < gridX; ++ix) {
                const auto a = static_cast<uint32_t>(numberOfVertices +  ix      + (gridX1 *  iy     ));
                const auto b = static_cast<uint32_t>(numberOfVertices +  ix      + (gridX1 * (iy + 1)));
                const auto c = static_cast<uint32_t>(numberOfVertices + (ix + 1) + (gridX1 * (iy + 1)));
                const auto d = static_cast<uint32_t>(numberOfVertices + (ix + 1) + (gridX1 *  iy     ));

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
    m_Mesh.setVerts(vertices);
    m_Mesh.setNormals(normals);
    m_Mesh.setTexCoords(uvs);
    m_Mesh.setIndices(indices);
    m_Mesh.setSubmeshDescriptors(submeshes);
}
