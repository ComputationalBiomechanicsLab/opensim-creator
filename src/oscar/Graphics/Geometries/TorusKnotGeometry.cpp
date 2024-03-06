#include "TorusKnotGeometry.h"

#include <oscar/Graphics/Mesh.h>
#include <oscar/Maths/Angle.h>
#include <oscar/Maths/GeometricFunctions.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/Maths/TrigonometricFunctions.h>

#include <cstddef>
#include <cstdint>
#include <vector>

using namespace osc;
using namespace osc::literals;

Mesh osc::TorusKnotGeometry::generate_mesh(
    float torusRadius,
    float tubeRadius,
    size_t numTubularSegments,
    size_t numRadialSegments,
    size_t p,
    size_t q)
{
    // the implementation/API of this was initially translated from `three.js`'s
    // `TorusKnotGeometry`, which has excellent documentation and source code. The
    // code was then subsequently mutated to suit OSC, C++ etc.
    //
    // https://threejs.org/docs/#api/en/geometries/TorusKnotGeometry

    auto const fNumTubularSegments = static_cast<float>(numTubularSegments);
    auto const fNumRadialSegments = static_cast<float>(numRadialSegments);
    auto const fp = static_cast<float>(p);
    auto const fq = static_cast<float>(q);

    // helper: calculates the current position on the torus curve
    auto const calculatePositionOnCurve = [fp, fq, torusRadius](Radians u)
    {
        Radians const quOverP = fq/fp * u;
        float const cs = cos(quOverP);

        return Vec3{
            torusRadius * (2.0f + cs) * 0.5f * cos(u),
            torusRadius * (2.0f + cs) * 0.5f * sin(u),
            torusRadius * sin(quOverP) * 0.5f,
        };
    };

    size_t const numVerts = (numTubularSegments+1)*(numRadialSegments+1);
    size_t const numIndices = 6*numTubularSegments*numRadialSegments;

    std::vector<uint32_t> indices;
    indices.reserve(numIndices);
    std::vector<Vec3> vertices;
    vertices.reserve(numVerts);
    std::vector<Vec3> normals;
    normals.reserve(numVerts);
    std::vector<Vec2> uvs;
    uvs.reserve(numVerts);

    // generate vertices, normals, and uvs
    for (size_t i = 0; i <= numTubularSegments; ++i) {
        auto const fi = static_cast<float>(i);

        // `u` is used to calculate the position on the torus curve of the current tubular segment
        Radians const u = fi/fNumTubularSegments * fp * 360_deg;

        // now we calculate two points. P1 is our current position on the curve, P2 is a little farther ahead.
        // these points are used to create a special "coordinate space", which is necessary to calculate the
        // correct vertex positions
        Vec3 const P1 = calculatePositionOnCurve(u);
        Vec3 const P2 = calculatePositionOnCurve(u + 0.01_rad);

        // calculate orthonormal basis
        Vec3 const T = P2 - P1;
        Vec3 N = P2 + P1;
        Vec3 B = cross(T, N);
        N = cross(B, T);

        // normalize B, N. T can be ignored, we don't use it
        B = normalize(B);
        N = normalize(N);

        for (size_t j = 0; j <= numRadialSegments; ++j) {
            auto const fj = static_cast<float>(j);

            // now calculate the vertices. they are nothing more than an extrusion of the torus curve.
            // because we extrude a shape in the xy-plane, there is no need to calculate a z-value.

            Radians const v = fj/fNumRadialSegments * 360_deg;
            float const cx = -tubeRadius * cos(v);
            float const cy =  tubeRadius * sin(v);

            // now calculate the final vertex position.
            // first we orient the extrusion with our basis vectors, then we add it to the current position on the curve
            Vec3 const vertex = {
                P1.x + (cx * N.x + cy * B.x),
                P1.y + (cx * N.y + cy * B.y),
                P1.z + (cx * N.z + cy * B.z),
            };
            vertices.push_back(vertex);

            // normal (P1 is always the center/origin of the extrusion, thus we can use it to calculate the normal)
            normals.push_back(normalize(vertex - P1));

            uvs.emplace_back(fi / fNumTubularSegments, fj / fNumRadialSegments);
        }
    }

    // generate indices
    for (size_t j = 1; j <= numTubularSegments; ++j) {
        for (size_t i = 1; i <= numRadialSegments; ++i) {
            auto const a = static_cast<uint32_t>((numRadialSegments + 1) * (j - 1) + (i - 1));
            auto const b = static_cast<uint32_t>((numRadialSegments + 1) *  j      + (i - 1));
            auto const c = static_cast<uint32_t>((numRadialSegments + 1) *  j      +  i);
            auto const d = static_cast<uint32_t>((numRadialSegments + 1) * (j - 1) +  i);

            indices.insert(indices.end(), {a, b, d});
            indices.insert(indices.end(), {b, c, d});
        }
    }

    // build geometry
    Mesh rv;
    rv.setVerts(vertices);
    rv.setNormals(normals);
    rv.setTexCoords(uvs);
    rv.setIndices(indices);
    return rv;
}
