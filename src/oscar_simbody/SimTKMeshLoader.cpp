#include "SimTKMeshLoader.h"

#include <oscar_simbody/SimTKConverters.h>

#include <SimTKcommon/internal/DecorativeGeometry.h>
#include <SimTKcommon/internal/PolygonalMesh.h>
#include <oscar/Graphics/Mesh.h>
#include <oscar/Graphics/MeshTopology.h>
#include <oscar/Graphics/VertexAttribute.h>
#include <oscar/Graphics/VertexAttributeFormat.h>
#include <oscar/Graphics/VertexFormat.h>
#include <oscar/Maths/MathHelpers.h>
#include <oscar/Maths/Triangle.h>
#include <oscar/Maths/TriangleFunctions.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/Utils/Assertions.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <ranges>
#include <string_view>
#include <vector>

using namespace osc;
using namespace std::literals;
namespace rgs = std::ranges;

namespace
{
    constexpr auto c_supported_mesh_extensions = std::to_array({"obj"sv, "vtp"sv, "stl"sv, "stla"sv});

    struct OutputMeshMetrics {
        size_t numVertices = 0;
        size_t numIndices = 0;
    };

    OutputMeshMetrics CalcMeshMetrics(const SimTK::PolygonalMesh& mesh)
    {
        OutputMeshMetrics rv;
        rv.numVertices = mesh.getNumVertices();
        for (int i = 0, faces = mesh.getNumFaces(); i < faces; ++i) {
            const int numFaceVerts = mesh.getNumVerticesForFace(i);
            if (numFaceVerts < 3) {
                continue;  // ignore lines/points
            }
            else if (numFaceVerts == 3) {
                rv.numIndices += 3;
            }
            else if (numFaceVerts == 4) {
                rv.numIndices += 6;
            }
            else {
                rv.numVertices += 1;  // triangulation
                rv.numIndices += static_cast<size_t>(3*(numFaceVerts-2));
            }
        }
        return rv;
    }
}

Mesh osc::ToOscMesh(const SimTK::PolygonalMesh& mesh)
{
    const auto metrics = CalcMeshMetrics(mesh);

    std::vector<Vec3> vertices;
    vertices.reserve(metrics.numVertices);

    std::vector<uint32_t> indices;
    indices.reserve(metrics.numIndices);

    // helper: validate+push triangle into the index list
    const auto pushTriangle = [&indices, &vertices](uint32_t a, uint32_t b, uint32_t c)
    {
        if (a >= vertices.size() || b >= vertices.size() || c >= vertices.size()) {
            return;  // index out-of-bounds
        }

        if (!can_form_triangle(vertices[a], vertices[b], vertices[c])) {
            return;  // vertex data doesn't form a triangle (NaNs, degenerate locations)
        }

        indices.insert(indices.end(), {a, b, c});
    };

    // copy all vertex positions from the source mesh
    for (int i = 0; i < mesh.getNumVertices(); ++i) {
        vertices.push_back(to<Vec3>(mesh.getVertexPosition(i)));
    }

    // build up the index list while triangulating any n>3 faces
    //
    // (pushes injected triangulation vertices to the end - assumes the mesh is optimized later)
    for (int face = 0, faces = mesh.getNumFaces(); face < faces; ++face) {
        const int numFaceVerts = mesh.getNumVerticesForFace(face);

        if (numFaceVerts <= 1) {
            // point (ignore)
            continue;
        }
        if (numFaceVerts == 2) {
            // line (ignore)
            continue;
        }
        else if (numFaceVerts == 3) {
            // triangle
            const auto a = static_cast<uint32_t>(mesh.getFaceVertex(face, 0));
            const auto b = static_cast<uint32_t>(mesh.getFaceVertex(face, 1));
            const auto c = static_cast<uint32_t>(mesh.getFaceVertex(face, 2));

            pushTriangle(a, b, c);
        }
        else if (numFaceVerts == 4) {
            // quad (emit as two triangles)
            const auto a = static_cast<uint32_t>(mesh.getFaceVertex(face, 0));
            const auto b = static_cast<uint32_t>(mesh.getFaceVertex(face, 1));
            const auto c = static_cast<uint32_t>(mesh.getFaceVertex(face, 2));
            const auto d = static_cast<uint32_t>(mesh.getFaceVertex(face, 3));

            pushTriangle(a, b, c);
            pushTriangle(a, c, d);
        }
        else {
            // polygon: triangulate each edge with a centroid

            // compute+add centroid vertex
            Vec3 centroid_of{};
            for (int vert = 0; vert < numFaceVerts; ++vert) {
                centroid_of += vertices.at(mesh.getFaceVertex(face, vert));
            }
            centroid_of /= static_cast<float>(numFaceVerts);
            const auto centroidIdx = static_cast<uint32_t>(vertices.size());
            vertices.push_back(centroid_of);

            // triangulate polygon loop
            for (int vert = 0; vert < numFaceVerts-1; ++vert) {
                const auto b = static_cast<uint32_t>(mesh.getFaceVertex(face, vert));
                const auto c = static_cast<uint32_t>(mesh.getFaceVertex(face, vert+1));

                pushTriangle(centroidIdx, b, c);
            }

            // (complete the loop)
            const auto b = static_cast<uint32_t>(mesh.getFaceVertex(face, numFaceVerts-1));
            const auto c = static_cast<uint32_t>(mesh.getFaceVertex(face, 0));
            pushTriangle(centroidIdx, b, c);
        }
    }

    Mesh rv;
    rv.set_vertices(vertices);
    rv.set_indices(indices);
    rv.recalculate_normals();
    return rv;
}

std::span<const std::string_view> osc::GetSupportedSimTKMeshFormats()
{
    return c_supported_mesh_extensions;
}

Mesh osc::LoadMeshViaSimTK(const std::filesystem::path& p)
{
    const SimTK::DecorativeMeshFile dmf{p.string()};
    const SimTK::PolygonalMesh& mesh = dmf.getMesh();
    return ToOscMesh(mesh);
}

void osc::AssignIndexedVerts(SimTK::PolygonalMesh& mesh, std::span<const Vec3> vertices, MeshIndicesView indices)
{
    mesh.clear();

    // assign vertices
    for (const Vec3& vertex : vertices) {
        mesh.addVertex(to<SimTK::Vec3>(vertex));
    }

    // assign indices (assumed triangle)
    OSC_ASSERT_ALWAYS(indices.size() % 3 == 0);
    SimTK::Array_<int> triVerts(3, 0);
    for (auto it = rgs::begin(indices); it != rgs::end(indices); it += 3) {
        triVerts[0] = static_cast<int>(it[0]);
        triVerts[1] = static_cast<int>(it[1]);
        triVerts[2] = static_cast<int>(it[2]);
        mesh.addFace(triVerts);
    }
}
