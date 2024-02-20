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
            if (rv.numIndices > 3) {  // handle triangulation
                ++rv.numVertices;
                ++rv.numIndices;
            }
        }
        return rv;
    }
}

Mesh osc::ToOscMesh(SimTK::PolygonalMesh const& mesh)
{
    // original source: simbody's `VisualizerProtocol.cpp:drawPolygonalMesh(...)`

    // figure out how much memory to allocate ahead of time
    auto const metrics = CalcMeshMetrics(mesh);

    // copy all vertex positions from the source mesh
    std::vector<Vec3> vertices;
    vertices.reserve(metrics.numVertices);
    for (int i = 0; i < mesh.getNumVertices(); ++i) {
        vertices.push_back(ToVec3(mesh.getVertexPosition(i)));
    }

    // build up the index list while triangulating any n>3 faces
    //
    // (puts the injected triangulation verts at the end and optimizes later)
    std::vector<uint32_t> indices;
    indices.reserve(metrics.numIndices);
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
            indices.insert(indices.end(), {a, b, c});
        }
        else if (numFaceVerts == 4) {
            // quad (emit as two triangles)
            auto const a = static_cast<uint32_t>(mesh.getFaceVertex(face, 0));
            auto const b = static_cast<uint32_t>(mesh.getFaceVertex(face, 1));
            auto const c = static_cast<uint32_t>(mesh.getFaceVertex(face, 2));
            auto const d = static_cast<uint32_t>(mesh.getFaceVertex(face, 3));
            indices.insert(indices.end(), {a, b, c,    a, c, d});
        }
        else {
            // polygon: triangulate each edge with a centroid

            // compute+add centroid vertex
            Vec3 centroid{};
            for (int vert = 0; vert < numFaceVerts; ++vert) {
                centroid += vertices.at(mesh.getFaceVertex(face, vert));
            }
            centroid /= static_cast<float>(numFaceVerts);
            uint32_t const centroidIdx = static_cast<uint32_t>(vertices.size());
            vertices.push_back(centroid);

            // triangulate polygon loop
            for (int vert = 0; vert < numFaceVerts-1; ++vert) {
                indices.insert(indices.end(), {
                    centroidIdx,
                    static_cast<uint32_t>(mesh.getFaceVertex(face, vert)),
                    static_cast<uint32_t>(mesh.getFaceVertex(face, vert+1)),
                });
            }
            // (complete the loop)
            indices.insert(indices.end(), {
                centroidIdx,
                static_cast<uint32_t>(mesh.getFaceVertex(face, numFaceVerts-1)),
                static_cast<uint32_t>(mesh.getFaceVertex(face, 0)),
            });
        }
    }

    Mesh rv;
    rv.setVerts(vertices);
    rv.setIndices(indices);
    //rv.optimize();
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
