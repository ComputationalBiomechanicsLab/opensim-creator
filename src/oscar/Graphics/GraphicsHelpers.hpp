#pragma once

#include <oscar/Graphics/ColorSpace.hpp>
#include <oscar/Graphics/ImageLoadingFlags.hpp>
#include <oscar/Graphics/MeshTopology.hpp>
#include <oscar/Graphics/Texture2D.hpp>
#include <oscar/Maths/Mat4.hpp>
#include <oscar/Maths/Sphere.hpp>
#include <oscar/Maths/Vec2.hpp>
#include <oscar/Maths/Vec3.hpp>
#include <oscar/Maths/Vec4.hpp>

#include <array>
#include <filesystem>
#include <functional>
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

    // returns a texture loaded from disk via Image
    //
    // throws if the image data isn't representable as a GPU texture (e.g. because it has
    // an incorrect number of color channels)
    Texture2D LoadTexture2DFromImage(
        std::filesystem::path const&,
        ColorSpace,
        ImageLoadingFlags = ImageLoadingFlags::None
    );

    void WriteToPNG(
        Texture2D const&,
        std::filesystem::path const&
    );

    // returns arrays that transforms cube faces from worldspace to projection
    // space such that the observer is looking at each face of the cube from
    // the center of the cube
    std::array<Mat4, 6> CalcCubemapViewProjMatrices(
        Mat4 const& projectionMatrix,
        Vec3 cubeCenter
    );

    // returns a list of vertices in mesh where each vertex was extracted as-if
    // by calling:
    //
    //   - mesh.getVertices()[mesh.getIndices()[i]];
    //
    // with range-checking on the indices (invalid indices are ignored)
    std::vector<Vec3> GetAllIndexedVerts(Mesh const&);

    // returns the bounding sphere of the given mesh
    Sphere BoundingSphereOf(Mesh const&);
}
