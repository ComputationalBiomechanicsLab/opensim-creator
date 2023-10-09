#pragma once

#include <oscar/Graphics/ColorSpace.hpp>
#include <oscar/Graphics/ImageLoadingFlags.hpp>
#include <oscar/Graphics/MeshTopology.hpp>
#include <oscar/Graphics/Texture2D.hpp>

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <nonstd/span.hpp>

#include <array>
#include <filesystem>
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
    glm::vec3 MassCenter(Mesh const&);

    // returns the average centerpoint of all vertices in a mesh
    glm::vec3 AverageCenterpoint(Mesh const&);

    // returns tangent vectors for the given (presumed, mesh) data
    //
    // the 4th (w) component of each vector indicates the flip direction
    // of the corresponding bitangent vector (i.e. bitangent = cross(normal, tangent) * w)
    std::vector<glm::vec4> CalcTangentVectors(
        MeshTopology const&,
        nonstd::span<glm::vec3 const> verts,
        nonstd::span<glm::vec3 const> normals,
        nonstd::span<glm::vec2 const> texCoords,
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
    std::array<glm::mat4, 6> CalcCubemapViewProjMatrices(
        glm::mat4 const& projectionMatrix,
        glm::vec3 cubeCenter
    );
}
