#include "SimTKMeshLoader.h"

#include <OpenSimCreator/Utils/SimTKHelpers.h>

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

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

using namespace osc;

// helper functions
namespace
{
    struct OutputMeshMetrics {
        size_t numVertices = 0;
        size_t numIndices = 0;
    };

    OutputMeshMetrics CalcMeshMetrics(SimTK::PolygonalMesh const& mesh)
    {
        OutputMeshMetrics rv;
        rv.numVertices = mesh.getNumVertices();
        for (int i = 0, faces = mesh.getNumFaces(); i < faces; ++i) {
            int const numFaceVerts = mesh.getNumVerticesForFace(i);
            if (numFaceVerts < 3) {
                continue;  // ignore lines/points
            }

            rv.numIndices = numFaceVerts;
            if (rv.numIndices > 4) {  // account for triangulation
                ++rv.numVertices;
                ++rv.numIndices;
            }
        }
        return rv;
    }
}

Mesh osc::ToOscMesh(SimTK::PolygonalMesh const& mesh)
{
    auto const metrics = CalcMeshMetrics(mesh);

    std::vector<Vec3> vertices;
    vertices.reserve(metrics.numVertices);

    std::vector<uint32_t> indices;
    indices.reserve(metrics.numIndices);

    // helper: validate+push triangle into the index list
    auto const pushTriangle = [&indices, &vertices](uint32_t a, uint32_t b, uint32_t c)
    {
        // validate that the indices actually form a triangle
        //
        // external mesh data can contain wacky problems, like bad indices
        // or repeated vertex locations that form lines, etc. - those problems
        // should stop here
        if (can_form_triangle(vertices.at(a), vertices.at(b), vertices.at(c))) {
            indices.insert(indices.end(), {a, b, c});
        }
    };

    // copy all vertex positions from the source mesh
    for (int i = 0; i < mesh.getNumVertices(); ++i) {
        vertices.push_back(ToVec3(mesh.getVertexPosition(i)));
    }

    // build up the index list while triangulating any n>3 faces
    //
    // (pushes injected triangulation verts to the end - assumes the mesh is optimized later)
    for (int face = 0, faces = mesh.getNumFaces(); face < faces; ++face) {
        int const numFaceVerts = mesh.getNumVerticesForFace(face);

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
            auto const a = static_cast<uint32_t>(mesh.getFaceVertex(face, 0));
            auto const b = static_cast<uint32_t>(mesh.getFaceVertex(face, 1));
            auto const c = static_cast<uint32_t>(mesh.getFaceVertex(face, 2));

            pushTriangle(a, b, c);
        }
        else if (numFaceVerts == 4) {
            // quad (emit as two triangles)
            auto const a = static_cast<uint32_t>(mesh.getFaceVertex(face, 0));
            auto const b = static_cast<uint32_t>(mesh.getFaceVertex(face, 1));
            auto const c = static_cast<uint32_t>(mesh.getFaceVertex(face, 2));
            auto const d = static_cast<uint32_t>(mesh.getFaceVertex(face, 3));

            pushTriangle(a, b, c);
            pushTriangle(a, c, d);
        }
        else {
            // polygon: triangulate each edge with a centroid

            // compute+add centroid vertex
            Vec3 centroid{};
            for (int vert = 0; vert < numFaceVerts; ++vert) {
                centroid += vertices.at(mesh.getFaceVertex(face, vert));
            }
            centroid /= static_cast<float>(numFaceVerts);
            auto const centroidIdx = static_cast<uint32_t>(vertices.size());
            vertices.push_back(centroid);

            // triangulate polygon loop
            for (int vert = 0; vert < numFaceVerts-1; ++vert) {
                auto const b = static_cast<uint32_t>(mesh.getFaceVertex(face, vert));
                auto const c = static_cast<uint32_t>(mesh.getFaceVertex(face, vert+1));

                pushTriangle(centroidIdx, b, c);
            }

            // (complete the loop)
            auto const b = static_cast<uint32_t>(mesh.getFaceVertex(face, numFaceVerts-1));
            auto const c = static_cast<uint32_t>(mesh.getFaceVertex(face, 0));
            pushTriangle(centroidIdx, b, c);
        }
    }

    Mesh rv;
    rv.setVerts(vertices);
    rv.setIndices(indices);
    rv.recalculateNormals();
    return rv;
}

std::string osc::GetCommaDelimitedListOfSupportedSimTKMeshFormats()
{
    return "obj,vtp,stl";
}

Mesh osc::LoadMeshViaSimTK(std::filesystem::path const& p)
{
    SimTK::DecorativeMeshFile const dmf{p.string()};
    SimTK::PolygonalMesh const& mesh = dmf.getMesh();
    return ToOscMesh(mesh);
}
