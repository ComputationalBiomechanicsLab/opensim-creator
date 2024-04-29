#pragma once

#include <oscar/Graphics/Mesh.h>
#include <oscar/Graphics/MeshTopology.h>
#include <oscar/Graphics/MeshIndicesView.h>
#include <oscar/Maths/Sphere.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/Maths/Vec4.h>

#include <span>
#include <vector>

namespace osc
{
    // returns the "mass center" of a mesh
    //
    // assumes:
    //
    // - the mesh volume has a constant density
    // - the mesh is entirely enclosed
    // - all mesh normals are correct
    Vec3 mass_center_of(const Mesh&);

    // returns the average centerpoint of all vertices in a mesh
    Vec3 average_centroid_of(const Mesh&);

    // returns tangent vectors for the given (presumed, mesh) data
    //
    // the 4th (w) component of each vector indicates the flip direction
    // of the corresponding bitangent vector (i.e. `bitangent = cross(normal, tangent) * w`)
    std::vector<Vec4> calc_tangent_vectors(
        const MeshTopology&,
        std::span<const Vec3> vertices,
        std::span<const Vec3> normals,
        std::span<const Vec2> tex_coords,
        const MeshIndicesView&
    );

    // returns the bounding sphere of the given mesh
    Sphere bounding_sphere_of(const Mesh&);
}
