#pragma once

#include <liboscar/graphics/Mesh.h>
#include <liboscar/graphics/MeshIndicesView.h>
#include <liboscar/graphics/MeshTopology.h>
#include <liboscar/maths/Sphere.h>
#include <liboscar/maths/Vector2.h>
#include <liboscar/maths/Vector3.h>
#include <liboscar/maths/Vector4.h>

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
    Vector3 mass_center_of(const Mesh&);

    // returns the average centerpoint of all vertices in a mesh
    Vector3 average_centroid_of(const Mesh&);

    // returns tangent vectors for the given (presumed, mesh) data
    //
    // the 4th (w) component of each vector indicates the flip direction
    // of the corresponding bitangent vector (i.e. `bitangent = cross(normal, tangent) * w`)
    std::vector<Vector4> calc_tangent_vectors(
        const MeshTopology&,
        std::span<const Vector3> vertices,
        std::span<const Vector3> normals,
        std::span<const Vector2> tex_coords,
        const MeshIndicesView&
    );

    // returns the bounding sphere of the given mesh
    std::optional<Sphere> bounding_sphere_of(const Mesh&);
}
