#include "GraphicsHelpers.hpp"

#include <oscar/Graphics/Color32.hpp>
#include <oscar/Graphics/ColorSpace.hpp>
#include <oscar/Graphics/ImageLoadingFlags.hpp>
#include <oscar/Graphics/Mesh.hpp>
#include <oscar/Graphics/MeshIndicesView.hpp>
#include <oscar/Graphics/MeshTopology.hpp>
#include <oscar/Graphics/Texture2D.hpp>
#include <oscar/Graphics/TextureFormat.hpp>
#include <oscar/Maths/Tetrahedron.hpp>
#include <oscar/Utils/Assertions.hpp>
#include <oscar/Utils/Cpp20Shims.hpp>
#include <oscar/Utils/SpanHelpers.hpp>

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/gtx/transform.hpp>
#include <nonstd/span.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace
{
    inline constexpr int c_StbTrue = 1;
    inline constexpr int c_StbFalse = 0;

    // this mutex is required because stbi has global mutable state (e.g. stbi_set_flip_vertically_on_load)
    auto LockStbiAPI()
    {
        static std::mutex s_StbiMutex;
        return std::lock_guard{s_StbiMutex};
    }

    osc::Texture2D Load32BitTexture(
        std::filesystem::path const& p,
        osc::ColorSpace colorSpace,
        osc::ImageLoadingFlags flags)
    {
        auto const guard = LockStbiAPI();

        if (flags & osc::ImageLoadingFlags::FlipVertically)
        {
            stbi_set_flip_vertically_on_load(c_StbTrue);
        }

        glm::ivec2 dims{};
        int numChannels = 0;
        std::unique_ptr<float, decltype(&stbi_image_free)> const pixels =
        {
            stbi_loadf(p.string().c_str(), &dims.x, &dims.y, &numChannels, 0),
            stbi_image_free,
        };

        if (flags & osc::ImageLoadingFlags::FlipVertically)
        {
            stbi_set_flip_vertically_on_load(c_StbFalse);
        }

        if (!pixels)
        {
            std::stringstream ss;
            ss << p << ": error loading image path: " << stbi_failure_reason();
            throw std::runtime_error{std::move(ss).str()};
        }

        std::optional<osc::TextureFormat> const format = osc::ToTextureFormat(
            static_cast<size_t>(numChannels),
            osc::TextureChannelFormat::Float32
        );

        if (!format)
        {
            std::stringstream ss;
            ss << p << ": error loading image: no TextureFormat exists for " << numChannels << " floating-point channel images";
            throw std::runtime_error{std::move(ss).str()};
        }

        nonstd::span<float const> const pixelSpan
        {
            pixels.get(),
            static_cast<size_t>(dims.x*dims.y*numChannels)
        };

        osc::Texture2D rv
        {
            dims,
            *format,
            colorSpace,
        };
        rv.setPixelData(osc::ViewSpanAsUint8Span(pixelSpan));

        return rv;
    }

    osc::Texture2D Load8BitTexture(
        std::filesystem::path const& p,
        osc::ColorSpace colorSpace,
        osc::ImageLoadingFlags flags)
    {
        auto const guard = LockStbiAPI();

        if (flags & osc::ImageLoadingFlags::FlipVertically)
        {
            stbi_set_flip_vertically_on_load(c_StbTrue);
        }

        glm::ivec2 dims{};
        int numChannels = 0;
        std::unique_ptr<stbi_uc, decltype(&stbi_image_free)> const pixels =
        {
            stbi_load(p.string().c_str(), &dims.x, &dims.y, &numChannels, 0),
            stbi_image_free,
        };

        if (flags & osc::ImageLoadingFlags::FlipVertically)
        {
            stbi_set_flip_vertically_on_load(c_StbFalse);
        }

        if (!pixels)
        {
            std::stringstream ss;
            ss << p << ": error loading image path: " << stbi_failure_reason();
            throw std::runtime_error{std::move(ss).str()};
        }

        std::optional<osc::TextureFormat> const format = osc::ToTextureFormat(
            static_cast<size_t>(numChannels),
            osc::TextureChannelFormat::Uint8
        );

        if (!format)
        {
            std::stringstream ss;
            ss << p << ": error loading image: no TextureFormat exists for " << numChannels << " 8-bit channel images";
            throw std::runtime_error{std::move(ss).str()};
        }

        osc::Texture2D rv
        {
            dims,
            *format,
            colorSpace,
        };
        rv.setPixelData({pixels.get(), static_cast<size_t>(dims.x*dims.y*numChannels)});
        return rv;
    }

    // describes the direction of each cube face and which direction is "up"
    // from the perspective of looking at that face from the center of the cube
    struct CubemapFaceDetails final {
        glm::vec3 direction;
        glm::vec3 up;
    };
    constexpr auto c_CubemapFacesDetails = osc::to_array<CubemapFaceDetails>(
    {
        {{ 1.0f,  0.0f,  0.0f}, {0.0f, -1.0f,  0.0f}},
        {{-1.0f,  0.0f,  0.0f}, {0.0f, -1.0f,  0.0f}},
        {{ 0.0f,  1.0f,  0.0f}, {0.0f,  0.0f,  1.0f}},
        {{ 0.0f, -1.0f,  0.0f}, {0.0f,  0.0f, -1.0f}},
        {{ 0.0f,  0.0f,  1.0f}, {0.0f, -1.0f,  0.0f}},
        {{ 0.0f,  0.0f, -1.0f}, {0.0f, -1.0f,  0.0f}},
    });

    glm::mat4 CalcCubemapViewMatrix(CubemapFaceDetails const& faceDetails, glm::vec3 const& cubeCenter)
    {
        return glm::lookAt(cubeCenter, cubeCenter + faceDetails.direction, faceDetails.up);
    }
}

glm::vec3 osc::MassCenter(Mesh const& m)
{
    // hastily implemented from: http://forums.cgsociety.org/t/how-to-calculate-center-of-mass-for-triangular-mesh/1309966
    //
    // effectively:
    //
    // - compute the centerpoint and volume of tetrahedrons created from
    //   some arbitrary point in space to each triangle in the mesh
    //
    // - compute the weighted sum: sum(volume * center) / sum(volume)
    //
    // this yields a 3D location that is a "mass center", *but* the volume
    // calculation is signed based on vertex winding (normal), so if the user
    // submits an invalid mesh, this calculation could potentially produce a
    // volume that's *way* off

    if (m.getTopology() != MeshTopology::Triangles)
    {
        return {0.0f, 0.0f, 0.0f};
    }

    nonstd::span<glm::vec3 const> const verts = m.getVerts();
    MeshIndicesView const indices = m.getIndices();
    size_t const len = (indices.size() / 3) * 3;  // paranioa

    double totalVolume = 0.0f;
    glm::dvec3 weightedCenterOfMass = {0.0, 0.0, 0.0};
    for (size_t i = 0; i < len; i += 3)
    {
        Tetrahedron tetrahedron
        {
            glm::vec3{0.0f, 0.0f, 0.0f},  // reference point
            verts[indices[i]],
            verts[indices[i+1]],
            verts[indices[i+2]],
        };

        double const volume = Volume(tetrahedron);
        glm::dvec3 const centerOfMass = Center(tetrahedron);

        totalVolume += volume;
        weightedCenterOfMass += volume * centerOfMass;
    }
    return weightedCenterOfMass / totalVolume;
}

glm::vec3 osc::AverageCenterpoint(Mesh const& m)
{
    MeshIndicesView const indices = m.getIndices();
    nonstd::span<glm::vec3 const> const verts = m.getVerts();

    glm::vec3 acc = {0.0f, 0.0f, 0.0f};
    for (uint32_t index : indices)
    {
        acc += verts[index];
    }
    acc /= static_cast<float>(verts.size());

    return acc;
}

std::vector<glm::vec4> osc::CalcTangentVectors(
    MeshTopology const& topology,
    nonstd::span<glm::vec3 const> verts,
    nonstd::span<glm::vec3 const> normals,
    nonstd::span<glm::vec2 const> texCoords,
    MeshIndicesView const& indices)
{
    // related:
    //
    // *initial source: https://learnopengl.com/Advanced-Lighting/Normal-Mapping
    // https://www.cs.utexas.edu/~fussell/courses/cs384g-spring2016/lectures/normal_mapping_tangent.pdf
    // https://gamedev.stackexchange.com/questions/68612/how-to-compute-tangent-and-bitangent-vectors
    // https://stackoverflow.com/questions/25349350/calculating-per-vertex-tangents-for-glsl
    // http://www.terathon.com/code/tangent.html
    // http://image.diku.dk/projects/media/morten.mikkelsen.08.pdf
    // http://www.crytek.com/download/Triangle_mesh_tangent_space_calculation.pdf

    std::vector<glm::vec4> rv;

    // edge-case: there's insufficient topological/normal/coordinate data, so
    //            return fallback-filled ({1,0,0,1}) vector
    if (topology != MeshTopology::Triangles ||
        normals.empty() ||
        texCoords.empty())
    {
        rv.assign(verts.size(), {1.0f, 0.0f, 0.0f, 1.0f});
        return rv;
    }

    // else: there must be enough data to compute the tangents
    //
    // (but, just to keep sane, assert that the mesh data is actually valid)
    OSC_ASSERT_ALWAYS(std::all_of(
        indices.begin(),
        indices.end(),
        [nVerts = verts.size(), nNormals = normals.size(), nCoords = texCoords.size()](auto index)
        {
            return index < nVerts && index < nNormals && index < nCoords;
        }
    ) && "the provided mesh contains invalid indices");

    // for smooth shading, vertices, normals, texture coordinates, and tangents
    // may be shared by multiple triangles. In this case, the tangents must be
    // averaged, so:
    //
    // - initialize all tangent vectors to `{0,0,0,0}`s
    // - initialize a weights vector filled with `0`s
    // - every time a tangent vector is computed:
    //     - accumulate a new average: `tangents[i] = (weights[i]*tangents[i] + newTangent)/weights[i]+1;`
    //     - increment weight: `weights[i]++`
    rv.assign(verts.size(), {0.0f, 0.0f, 0.0f, 0.0f});
    std::vector<uint16_t> weights(verts.size(), 0);
    auto const accumulateTangent = [&rv, &weights](auto i, glm::vec4 const& newTangent)
    {
        rv[i] = (static_cast<float>(weights[i])*rv[i] + newTangent)/(static_cast<float>(weights[i]+1));
        weights[i]++;
    };

    // compute tangent vectors from triangle primitives
    for (ptrdiff_t triBegin = 0, end = ssize(indices)-2; triBegin < end; triBegin += 3)
    {
        // compute edge vectors in object and tangent (UV) space
        glm::vec3 const e1 = verts[indices[triBegin+1]] - verts[indices[triBegin+0]];
        glm::vec3 const e2 = verts[indices[triBegin+2]] - verts[indices[triBegin+0]];
        glm::vec2 const dUV1 = texCoords[indices[triBegin+1]] - texCoords[indices[triBegin+0]];  // delta UV for edge 1
        glm::vec2 const dUV2 = texCoords[indices[triBegin+2]] - texCoords[indices[triBegin+0]];

        // this is effectively inline-ing a matrix inversion + multiplication, see:
        //
        // - https://www.cs.utexas.edu/~fussell/courses/cs384g-spring2016/lectures/normal_mapping_tangent.pdf
        // - https://learnopengl.com/Advanced-Lighting/Normal-Mapping
        float const invDeterminant = 1.0f/(dUV1.x*dUV2.y - dUV2.x*dUV1.y);
        glm::vec3 const tangent = invDeterminant * glm::vec3
        {
            dUV2.y*e1.x - dUV1.y*e2.x,
            dUV2.y*e1.y - dUV1.y*e2.y,
            dUV2.y*e1.z - dUV1.y*e2.z,
        };
        glm::vec3 const bitangent = invDeterminant * glm::vec3
        {
            -dUV2.x*e1.x + dUV1.x*e2.x,
            -dUV2.x*e1.y + dUV1.x*e2.y,
            -dUV2.x*e1.z + dUV1.x*e2.z,
        };

        // care: due to smooth shading, each normal may not actually be orthogonal
        // to the triangle's surface
        for (ptrdiff_t iVert = 0; iVert < 3; ++iVert)
        {
            auto const triVertIndex = indices[triBegin + iVert];

            // Gram-Schmidt orthogonalization (w.r.t. the stored normal)
            glm::vec3 const normal = glm::normalize(normals[triVertIndex]);
            glm::vec3 const orthoTangent = glm::normalize(tangent - glm::dot(normal, tangent)*normal);
            glm::vec3 const orthoBitangent = glm::normalize(bitangent - (glm::dot(orthoTangent, bitangent)*orthoTangent) - (glm::dot(normal, bitangent)*normal));

            // this algorithm doesn't produce bitangents. Instead, it writes the
            // "direction" (flip) of the bitangent w.r.t. `cross(normal, tangent)`
            //
            // (the shader can recompute the bitangent from: `cross(normal, tangent) * w`)
            float const w = glm::dot(glm::cross(normal, orthoTangent), orthoBitangent);

            accumulateTangent(triVertIndex, glm::vec4{orthoTangent, w});
        }
    }
    return rv;
}

osc::Texture2D osc::LoadTexture2DFromImage(
    std::filesystem::path const& path,
    ColorSpace colorSpace,
    ImageLoadingFlags flags)
{
    return stbi_is_hdr(path.string().c_str()) ?
        Load32BitTexture(path, colorSpace, flags) :
        Load8BitTexture(path, colorSpace, flags);
}

void osc::WriteToPNG(
    Texture2D const& tex,
    std::filesystem::path const& outpath)
{
    glm::ivec2 const dims = tex.getDimensions();
    int const stride = 4 * dims.x;
    std::vector<Color32> const pixels = tex.getPixels32();

    auto const guard = LockStbiAPI();
    stbi_flip_vertically_on_write(c_StbTrue);
    int const rv = stbi_write_png(
        outpath.string().c_str(),
        dims.x,
        dims.y,
        4,
        pixels.data(),
        stride
    );
    stbi_flip_vertically_on_write(c_StbFalse);

    OSC_ASSERT(rv != 0);
}

std::array<glm::mat4, 6> osc::CalcCubemapViewProjMatrices(
    glm::mat4 const& projectionMatrix,
    glm::vec3 cubeCenter)
{
    static_assert(std::size(c_CubemapFacesDetails) == 6);

    std::array<glm::mat4, 6> rv{};
    for (size_t i = 0; i < 6; ++i)
    {
        rv[i] = projectionMatrix * CalcCubemapViewMatrix(c_CubemapFacesDetails[i], cubeCenter);
    }
    return rv;
}

void osc::ForEachIndexedVert(Mesh const& mesh, std::function<void(glm::vec3)> const& callback)
{
    auto const verts = mesh.getVerts();
    auto const indices = mesh.getIndices();
    for (size_t i = 0; i < indices.size(); ++i)
    {
        static_assert(std::is_unsigned_v<decltype(indices[i])>);
        if (auto index = indices[i]; index < verts.size())
        {
            callback(verts[index]);
        }
    }
}

std::vector<glm::vec3> osc::GetAllIndexedVerts(Mesh const& mesh)
{
    auto const verts = mesh.getVerts();
    auto const indices = mesh.getIndices();

    std::vector<glm::vec3> rv;
    rv.reserve(indices.size());  // guess: it's likely that all indices are fine
    for (size_t i = 0; i < indices.size(); ++i)
    {
        static_assert(std::is_unsigned_v<decltype(indices[i])>);
        if (auto index = indices[i]; index < verts.size())
        {
            rv.push_back(verts[index]);
        }
    }
    return rv;
}
