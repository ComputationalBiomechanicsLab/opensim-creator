#include "SimTKMeshLoader.hpp"

#include "OpenSimCreator/Bindings/SimTKHelpers.hpp"

#include <oscar/Graphics/Mesh.hpp>
#include <oscar/Graphics/MeshTopology.hpp>
#include <oscar/Maths/MathHelpers.hpp>
#include <oscar/Maths/Triangle.hpp>

#include <glm/vec3.hpp>
#include <SimTKcommon/internal/DecorativeGeometry.h>
#include <SimTKcommon/internal/PolygonalMesh.h>

#include <cstddef>
#include <cstdint>
#include <vector>

// helper functions
namespace
{
    // get the `vert`th vertex of the `face`th face
    glm::vec3 GetFaceVertex(SimTK::PolygonalMesh const& mesh, int face, int vert)
    {
        int const vertidx = mesh.getFaceVertex(face, vert);
        SimTK::Vec3 const& pos = mesh.getVertexPosition(vertidx);

        return osc::ToVec3(pos);
    }
}

osc::Mesh osc::ToOscMesh(SimTK::PolygonalMesh const& mesh)
{
    // see: simbody VisualizerProtocol.cpp:drawPolygonalMesh(...) for what this is
    // roughly based on

    std::vector<glm::vec3> verts;
    std::vector<glm::vec3> normals;
    std::vector<uint32_t> indices;

    verts.reserve(static_cast<size_t>(mesh.getNumVertices()));
    normals.reserve(static_cast<size_t>(mesh.getNumVertices()));
    indices.reserve(static_cast<size_t>(mesh.getNumVertices()));

    uint32_t index = 0;
    auto const pushTriangle = [&verts, &normals, &indices, &index](osc::Triangle const& tri)
    {
        glm::vec3 const normal = osc::TriangleNormal(tri);

        for (size_t i = 0; i < 3; ++i)
        {
            verts.push_back(tri[i]);
            normals.push_back(normal);
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

            osc::Triangle const triangle =
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

            glm::vec3 const quadVerts[4] =
            {
                GetFaceVertex(mesh, face, 0),
                GetFaceVertex(mesh, face, 1),
                GetFaceVertex(mesh, face, 2),
                GetFaceVertex(mesh, face, 3),
            };

            pushTriangle({quadVerts[0], quadVerts[1], quadVerts[2]});
            pushTriangle({quadVerts[0], quadVerts[2], quadVerts[3]});
        }
        else
        {
            // polygon (>3 edges):
            //
            // create a vertex at the average center point and attach
            // every two verices to the center as triangles.

            glm::vec3 center = {0.0f, 0.0f, 0.0f};
            for (int vert = 0; vert < nVerts; ++vert)
            {
                center += GetFaceVertex(mesh, face, vert);
            }
            center /= nVerts;

            for (int vert = 0; vert < nVerts - 1; ++vert)
            {
                osc::Triangle const tri =
                {
                    center,
                    GetFaceVertex(mesh, face, vert),
                    GetFaceVertex(mesh, face, vert + 1),
                };
                pushTriangle(tri);
            }

            // complete the polygon loop
            osc::Triangle const tri =
            {
                center,
                GetFaceVertex(mesh, face, nVerts - 1),
                GetFaceVertex(mesh, face, 0),
            };
            pushTriangle(tri);
        }
    }

    osc::Mesh rv;
    rv.setTopology(osc::MeshTopology::Triangles);
    rv.setVerts(verts);
    rv.setNormals(normals);
    rv.setIndices(indices);
    return rv;
}

std::string osc::GetCommaDelimitedListOfSupportedSimTKMeshFormats()
{
    return "obj,vtp,stl";
}

osc::Mesh osc::LoadMeshViaSimTK(std::filesystem::path const& p)
{
    SimTK::DecorativeMeshFile const dmf{p.string()};
    SimTK::PolygonalMesh const& mesh = dmf.getMesh();
    return ToOscMesh(mesh);
}