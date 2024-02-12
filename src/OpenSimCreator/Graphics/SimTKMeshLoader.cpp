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
    // get the `vert`th vertex of the `face`th face
    Vec3 GetFaceVertex(SimTK::PolygonalMesh const& mesh, int face, int vert)
    {
        int const vertidx = mesh.getFaceVertex(face, vert);
        SimTK::Vec3 const& pos = mesh.getVertexPosition(vertidx);

        return ToVec3(pos);
    }
}

Mesh osc::ToOscMesh(SimTK::PolygonalMesh const& mesh)
{
    // see: simbody VisualizerProtocol.cpp:drawPolygonalMesh(...) for what this is
    // roughly based on

    size_t const numVerts = mesh.getNumVertices();

    struct Vertex final {
        Vec3 position;
        Vec3 normal;
    };
    std::vector<Vertex> verts;
    verts.reserve(numVerts);

    std::vector<uint32_t> indices;
    indices.reserve(numVerts);

    uint32_t index = 0;
    auto const pushTriangle = [&verts, &indices, &index](Triangle const& tri)
    {
        Vec3 const normal = TriangleNormal(tri);

        for (size_t i = 0; i < 3; ++i)
        {
            verts.push_back({.position = tri[i], .normal = normal });
            indices.push_back(index++);
        }
    };

    for (int face = 0, nfaces = mesh.getNumFaces(); face < nfaces; ++face)
    {
        int const nVerts = mesh.getNumVerticesForFace(face);

        if (nVerts < 3)
        {
            // line/point (ignore it)
        }
        else if (nVerts == 3)
        {
            // triangle

            Triangle const triangle =
            {
                GetFaceVertex(mesh, face, 0),
                GetFaceVertex(mesh, face, 1),
                GetFaceVertex(mesh, face, 2),
            };
            pushTriangle(triangle);
        }
        else if (nVerts == 4)
        {
            // quad (render as two triangles)

            auto const quadVerts = std::to_array<Vec3>(
            {
                GetFaceVertex(mesh, face, 0),
                GetFaceVertex(mesh, face, 1),
                GetFaceVertex(mesh, face, 2),
                GetFaceVertex(mesh, face, 3),
            });

            pushTriangle({quadVerts[0], quadVerts[1], quadVerts[2]});
            pushTriangle({quadVerts[0], quadVerts[2], quadVerts[3]});
        }
        else
        {
            // polygon (>3 edges):
            //
            // create a vertex at the average center point and attach
            // every two verices to the center as triangles.

            Vec3 center = {0.0f, 0.0f, 0.0f};
            for (int vert = 0; vert < nVerts; ++vert)
            {
                center += GetFaceVertex(mesh, face, vert);
            }
            center /= nVerts;

            for (int vert = 0; vert < nVerts - 1; ++vert)
            {
                Triangle const tri =
                {
                    center,
                    GetFaceVertex(mesh, face, vert),
                    GetFaceVertex(mesh, face, vert + 1),
                };
                pushTriangle(tri);
            }

            // complete the polygon loop
            Triangle const tri =
            {
                center,
                GetFaceVertex(mesh, face, nVerts - 1),
                GetFaceVertex(mesh, face, 0),
            };
            pushTriangle(tri);
        }
    }

    Mesh rv;
    rv.setTopology(MeshTopology::Triangles);
    rv.setVertexBufferParams(verts.size(), {
        {VertexAttribute::Position, VertexAttributeFormat::Float32x3},
        {VertexAttribute::Normal, VertexAttributeFormat::Float32x3},
    });
    rv.setVertexBufferData(verts);
    rv.setIndices(indices);
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
