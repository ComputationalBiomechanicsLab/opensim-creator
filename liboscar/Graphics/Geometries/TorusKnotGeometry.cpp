#include "TorusKnotGeometry.h"

#include <liboscar/Graphics/Mesh.h>
#include <liboscar/Maths/Angle.h>
#include <liboscar/Maths/GeometricFunctions.h>
#include <liboscar/Maths/Vec3.h>
#include <liboscar/Maths/TrigonometricFunctions.h>

#include <cstddef>
#include <cstdint>
#include <vector>

using namespace osc;
using namespace osc::literals;

osc::TorusKnotGeometry::TorusKnotGeometry(const Params& p)
{
    // the implementation/API of this was initially translated from `three.js`'s
    // `TorusKnotGeometry`, which has excellent documentation and source code. The
    // code was then subsequently mutated to suit OSC, C++ etc.
    //
    // https://threejs.org/docs/#api/en/geometries/TorusKnotGeometry

    const auto fnum_tubular_segments = static_cast<float>(p.num_tubular_segments);
    const auto fnum_radial_segments = static_cast<float>(p.num_radial_segments);
    const auto fp = static_cast<float>(p.p);
    const auto fq = static_cast<float>(p.q);

    // helper: calculates the current position on the torus curve
    const auto calc_position_on_curve = [fp, fq, &p](Radians u)
    {
        const Radians qu_over_p = fq/fp * u;
        const float cs = cos(qu_over_p);

        return Vec3{
            p.torus_radius * (2.0f + cs) * 0.5f * cos(u),
            p.torus_radius * (2.0f + cs) * 0.5f * sin(u),
            p.torus_radius * sin(qu_over_p) * 0.5f,
        };
    };

    const size_t num_vertices = (p.num_tubular_segments+1)*(p.num_radial_segments+1);
    const size_t num_indices = 6*p.num_tubular_segments*p.num_radial_segments;

    std::vector<uint32_t> indices;
    indices.reserve(num_indices);
    std::vector<Vec3> vertices;
    vertices.reserve(num_vertices);
    std::vector<Vec3> normals;
    normals.reserve(num_vertices);
    std::vector<Vec2> uvs;
    uvs.reserve(num_vertices);

    // generate vertices, normals, and uvs
    for (size_t i = 0; i <= p.num_tubular_segments; ++i) {
        const auto fi = static_cast<float>(i);

        // `u` is used to calculate the position on the torus curve of the current tubular segment
        const Radians u = fi/fnum_tubular_segments * fp * 360_deg;

        // now we calculate two points. p1 is our current position on the curve, p2 is a little farther ahead.
        // these points are used to create a special "coordinate space", which is necessary to calculate the
        // correct vertex positions
        const Vec3 p1 = calc_position_on_curve(u);
        const Vec3 p2 = calc_position_on_curve(u + 0.01_rad);

        // calculate orthonormal basis
        const Vec3 T = p2 - p1;
        Vec3 N = p2 + p1;
        Vec3 B = cross(T, N);
        N = cross(B, T);

        // normalize B, N. T can be ignored, we don't use it
        B = normalize(B);
        N = normalize(N);

        for (size_t j = 0; j <= p.num_radial_segments; ++j) {
            const auto fj = static_cast<float>(j);

            // now calculate the vertices. they are nothing more than an extrusion of the torus curve.
            // because we extrude a shape in the xy-plane, there is no need to calculate a z-value.

            const Radians v = fj/fnum_radial_segments * 360_deg;
            const float cx = -p.tube_radius * cos(v);
            const float cy =  p.tube_radius * sin(v);

            // now calculate the final vertex position.
            // first we orient the extrusion with our basis vectors, then we add it to the current position on the curve
            const Vec3 vertex = {
                p1.x + (cx * N.x + cy * B.x),
                p1.y + (cx * N.y + cy * B.y),
                p1.z + (cx * N.z + cy * B.z),
            };
            vertices.push_back(vertex);

            // normal (p1 is always the center/origin of the extrusion, thus we can use it to calculate the normal)
            normals.push_back(normalize(vertex - p1));

            uvs.emplace_back(fi / fnum_tubular_segments, fj / fnum_radial_segments);
        }
    }

    // generate indices
    for (size_t j = 1; j <= p.num_tubular_segments; ++j) {
        for (size_t i = 1; i <= p.num_radial_segments; ++i) {
            const auto a = static_cast<uint32_t>((p.num_radial_segments + 1) * (j - 1) + (i - 1));
            const auto b = static_cast<uint32_t>((p.num_radial_segments + 1) *  j      + (i - 1));
            const auto c = static_cast<uint32_t>((p.num_radial_segments + 1) *  j      +  i);
            const auto d = static_cast<uint32_t>((p.num_radial_segments + 1) * (j - 1) +  i);

            indices.insert(indices.end(), {a, b, d});
            indices.insert(indices.end(), {b, c, d});
        }
    }

    // build geometry
    set_vertices(vertices);
    set_normals(normals);
    set_tex_coords(uvs);
    set_indices(indices);
}
