#pragma once

#include <oscar/Graphics/MeshTopology.h>
#include <oscar/Maths/Mat4.h>
#include <oscar/Maths/Sphere.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/Maths/Vec4.h>

#include <array>
#include <span>
#include <vector>

namespace osc { class Mesh; }
namespace osc { class MeshIndicesView; }

namespace osc
{
    // returns the "mass center" of a mesh
    //
    // assumes:
    //
    // - the mesh volume has a constant density
    // - the mesh is entirely enclosed
    // - all mesh normals are correct
    Vec3 MassCenter(Mesh const&);

    // returns the average centerpoint of all vertices in a mesh
    Vec3 AverageCenterpoint(Mesh const&);

    // returns tangent vectors for the given (presumed, mesh) data
    //
    // the 4th (w) component of each vector indicates the flip direction
    // of the corresponding bitangent vector (i.e. bitangent = cross(normal, tangent) * w)
    std::vector<Vec4> CalcTangentVectors(
        MeshTopology const&,
        std::span<Vec3 const> verts,
        std::span<Vec3 const> normals,
        std::span<Vec2 const> texCoords,
        MeshIndicesView const&
    );

    // returns arrays that transforms cube faces from worldspace to projection
    // space such that the observer is looking at each face of the cube from
    // the center of the cube
    std::array<Mat4, 6> CalcCubemapViewProjMatrices(
        Mat4 const& projectionMatrix,
        Vec3 cubeCenter
    );

    // returns the bounding sphere of the given mesh
    Sphere BoundingSphereOf(Mesh const&);
}
