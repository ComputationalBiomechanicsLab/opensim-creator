#include "MeshFunctions.h"

#include <oscar/Graphics/Mesh.h>
#include <oscar/Graphics/MeshTopology.h>
#include <oscar/Graphics/MeshIndicesView.h>
#include <oscar/Maths/GeometricFunctions.h>
#include <oscar/Maths/MathHelpers.h>
#include <oscar/Maths/Sphere.h>
#include <oscar/Maths/Tetrahedron.h>
#include <oscar/Maths/Triangle.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/Maths/Vec4.h>
#include <oscar/Utils/Algorithms.h>
#include <oscar/Utils/Assertions.h>

#include <cstddef>
#include <cstdint>
#include <vector>

using namespace osc;

Vec3 osc::AverageCenterpoint(Mesh const& m)
{
    Vec3 accumulator{};
    size_t i = 0;
    m.forEachIndexedVert([&accumulator, &i](Vec3 v)
    {
        accumulator += v;
        ++i;
    });
    return accumulator / static_cast<float>(i);
}

std::vector<Vec4> osc::CalcTangentVectors(
    MeshTopology const& topology,
    std::span<Vec3 const> verts,
    std::span<Vec3 const> normals,
    std::span<Vec2 const> texCoords,
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

    std::vector<Vec4> rv;

    // edge-case: there's insufficient topological/normal/coordinate data, so
    //            return fallback-filled ({1,0,0,1}) vector
    if (topology != MeshTopology::Triangles ||
        normals.empty() ||
        texCoords.empty()) {

        rv.assign(verts.size(), {1.0f, 0.0f, 0.0f, 1.0f});
        return rv;
    }

    // else: there must be enough data to compute the tangents
    //
    // (but, just to keep sane, assert that the mesh data is actually valid)
    OSC_ASSERT_ALWAYS(all_of(indices, [nVerts = verts.size(), nNormals = normals.size(), nCoords = texCoords.size()](auto index)
    {
        return index < nVerts && index < nNormals && index < nCoords;
    }) && "the provided mesh contains invalid indices");

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
    auto const accumulateTangent = [&rv, &weights](auto i, Vec4 const& newTangent)
    {
        rv[i] = (static_cast<float>(weights[i])*rv[i] + newTangent)/(static_cast<float>(weights[i]+1));
        weights[i]++;
    };

    // compute tangent vectors from triangle primitives
    for (ptrdiff_t triBegin = 0, end = std::ssize(indices)-2; triBegin < end; triBegin += 3) {

        // compute edge vectors in object and tangent (UV) space
        Vec3 const e1 = verts[indices[triBegin+1]] - verts[indices[triBegin+0]];
        Vec3 const e2 = verts[indices[triBegin+2]] - verts[indices[triBegin+0]];
        Vec2 const dUV1 = texCoords[indices[triBegin+1]] - texCoords[indices[triBegin+0]];  // delta UV for edge 1
        Vec2 const dUV2 = texCoords[indices[triBegin+2]] - texCoords[indices[triBegin+0]];

        // this is effectively inline-ing a matrix inversion + multiplication, see:
        //
        // - https://www.cs.utexas.edu/~fussell/courses/cs384g-spring2016/lectures/normal_mapping_tangent.pdf
        // - https://learnopengl.com/Advanced-Lighting/Normal-Mapping
        float const invDeterminant = 1.0f/(dUV1.x*dUV2.y - dUV2.x*dUV1.y);
        Vec3 const tangent = invDeterminant * Vec3{
            dUV2.y*e1.x - dUV1.y*e2.x,
            dUV2.y*e1.y - dUV1.y*e2.y,
            dUV2.y*e1.z - dUV1.y*e2.z,
        };
        Vec3 const bitangent = invDeterminant * Vec3{
            -dUV2.x*e1.x + dUV1.x*e2.x,
            -dUV2.x*e1.y + dUV1.x*e2.y,
            -dUV2.x*e1.z + dUV1.x*e2.z,
        };

        // care: due to smooth shading, each normal may not actually be orthogonal
        // to the triangle's surface
        for (ptrdiff_t iVert = 0; iVert < 3; ++iVert) {
            auto const triVertIndex = indices[triBegin + iVert];

            // Gram-Schmidt orthogonalization (w.r.t. the stored normal)
            Vec3 const normal = normalize(normals[triVertIndex]);
            Vec3 const orthoTangent = normalize(tangent - dot(normal, tangent)*normal);
            Vec3 const orthoBitangent = normalize(bitangent - (dot(orthoTangent, bitangent)*orthoTangent) - (dot(normal, bitangent)*normal));

            // this algorithm doesn't produce bitangents. Instead, it writes the
            // "direction" (flip) of the bitangent w.r.t. `cross(normal, tangent)`
            //
            // (the shader can recompute the bitangent from: `cross(normal, tangent) * w`)
            float const w = dot(cross(normal, orthoTangent), orthoBitangent);

            accumulateTangent(triVertIndex, Vec4{orthoTangent, w});
        }
    }
    return rv;
}

Vec3 osc::MassCenter(Mesh const& m)
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

    if (m.getTopology() != MeshTopology::Triangles || m.getNumVerts() < 3) {
        return {0.0f, 0.0f, 0.0f};
    }

    double totalVolume = 0.0f;
    Vec3d weightedCenterOfMass{};
    m.forEachIndexedTriangle([&totalVolume, &weightedCenterOfMass](Triangle t)
    {
        Vec3 const referencePoint{};
        Tetrahedron const tetrahedron{referencePoint, t.p0, t.p1, t.p2};
        double const v = volume(tetrahedron);
        Vec3d const centerOfMass = centroid(tetrahedron);

        totalVolume += v;
        weightedCenterOfMass += v * centerOfMass;
    });
    return weightedCenterOfMass / totalVolume;
}

Sphere osc::BoundingSphereOf(Mesh const& mesh)
{
    return BoundingSphereOf(mesh.getVerts());
}
