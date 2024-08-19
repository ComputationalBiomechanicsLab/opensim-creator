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
#include <ranges>
#include <vector>

using namespace osc;
namespace ranges = std::ranges;

Vec3 osc::average_centroid_of(const Mesh& mesh)
{
    Vec3d accumulator{};
    size_t i = 0;
    mesh.for_each_indexed_vertex([&accumulator, &i](Vec3 v)
    {
        accumulator += v;
        ++i;
    });
    return Vec3{accumulator / static_cast<double>(i)};
}

std::vector<Vec4> osc::calc_tangent_vectors(
    const MeshTopology& topology,
    std::span<const Vec3> vertices,
    std::span<const Vec3> normals,
    std::span<const Vec2> tex_coords,
    const MeshIndicesView& indices)
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
    if (topology != MeshTopology::Triangles or
        normals.empty() or
        tex_coords.empty() or
        indices.size() < 3) {

        rv.assign(vertices.size(), {1.0f, 0.0f, 0.0f, 1.0f});
        return rv;
    }

    // else: there must be enough data to compute the tangents
    //
    // (but, just to keep sane, assert that the mesh data is actually valid)
    OSC_ASSERT_ALWAYS(ranges::all_of(indices, [&vertices, &normals, &tex_coords](auto index)
    {
        return index < vertices.size() and index < normals.size() and index < tex_coords.size();
    }) && "the provided mesh contains invalid indices");

    // for smooth shading, vertices, normals, texture coordinates, and tangents
    // may be shared by multiple triangles. In this case, the tangents must be
    // averaged, so:
    //
    // - initialize all tangent vectors to `{0,0,0,0}`s
    // - initialize a weights vector filled with `0`s
    // - every time a tangent vector is computed:
    //     - accumulate a new average: `tangents[i] = (weights[i]*tangents[i] + new_tangent)/weights[i]+1;`
    //     - increment weight: `weights[i]++`
    rv.assign(vertices.size(), Vec4{});
    std::vector<uint16_t> weights(vertices.size());
    const auto accumulate_tangent = [&rv, &weights](auto i, const Vec4& new_tangent)
    {
        rv[i] = (static_cast<float>(weights[i])*rv[i] + new_tangent)/(static_cast<float>(weights[i]+1));
        weights[i]++;
    };

    // compute tangent vectors from triangle primitives
    OSC_ASSERT(indices.size() >= 3 && "insufficient indices provided to compute triangle tangents");  // checked above
    for (size_t triangle_begin = 0, triangle_end = indices.size()-2; triangle_begin < triangle_end; triangle_begin += 3) {

        // compute edge vectors in object and tangent (UV) space
        const Vec3 e1 = vertices[indices[triangle_begin+1]] - vertices[indices[triangle_begin+0]];
        const Vec3 e2 = vertices[indices[triangle_begin+2]] - vertices[indices[triangle_begin+0]];
        const Vec2 delta_uv1 = tex_coords[indices[triangle_begin+1]] - tex_coords[indices[triangle_begin+0]];
        const Vec2 delta_uv2 = tex_coords[indices[triangle_begin+2]] - tex_coords[indices[triangle_begin+0]];

        // this is effectively inline-ing a matrix inversion + multiplication, see:
        //
        // - https://www.cs.utexas.edu/~fussell/courses/cs384g-spring2016/lectures/normal_mapping_tangent.pdf
        // - https://learnopengl.com/Advanced-Lighting/Normal-Mapping
        const float inv_determinant = 1.0f/(delta_uv1.x*delta_uv2.y - delta_uv2.x*delta_uv1.y);
        const Vec3 tangent = inv_determinant * Vec3{
            delta_uv2.y*e1.x - delta_uv1.y*e2.x,
            delta_uv2.y*e1.y - delta_uv1.y*e2.y,
            delta_uv2.y*e1.z - delta_uv1.y*e2.z,
        };
        const Vec3 bitangent = inv_determinant * Vec3{
            -delta_uv2.x*e1.x + delta_uv1.x*e2.x,
            -delta_uv2.x*e1.y + delta_uv1.x*e2.y,
            -delta_uv2.x*e1.z + delta_uv1.x*e2.z,
        };

        // care: due to smooth shading, each normal may not actually be orthogonal
        // to the triangle's surface
        for (size_t ith_vertex = 0; ith_vertex < 3; ++ith_vertex) {
            const auto triangle_vertex_index = indices[triangle_begin + ith_vertex];

            // Gram-Schmidt orthogonalization (w.r.t. the stored normal)
            const Vec3 normal = normalize(normals[triangle_vertex_index]);
            const Vec3 ortho_tangent = normalize(tangent - dot(normal, tangent)*normal);
            const Vec3 ortho_bitangent = normalize(bitangent - (dot(ortho_tangent, bitangent)*ortho_tangent) - (dot(normal, bitangent)*normal));

            // this algorithm doesn't produce bitangents. Instead, it writes the
            // "direction" (flip) of the bitangent w.r.t. `cross(normal, tangent)`
            //
            // (the shader can recompute the bitangent from: `cross(normal, tangent) * w`)
            const float w = dot(cross(normal, ortho_tangent), ortho_bitangent);

            accumulate_tangent(triangle_vertex_index, Vec4{ortho_tangent, w});
        }
    }
    return rv;
}

Vec3 osc::mass_center_of(const Mesh& mesh)
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

    if (mesh.topology() != MeshTopology::Triangles or mesh.num_vertices() < 3) {
        return Vec3{};
    }

    double total_volume = 0.0f;
    Vec3d weighted_com{};
    mesh.for_each_indexed_triangle([&total_volume, &weighted_com](Triangle t)
    {
        const Vec3 reference_point{};
        const Tetrahedron tetrahedron{reference_point, t.p0, t.p1, t.p2};
        const double v = volume_of(tetrahedron);
        const Vec3d com = centroid_of(tetrahedron);

        total_volume += v;
        weighted_com += v * com;
    });
    return weighted_com / total_volume;
}

Sphere osc::bounding_sphere_of(const Mesh& mesh)
{
    return bounding_sphere_of(mesh.vertices());
}
