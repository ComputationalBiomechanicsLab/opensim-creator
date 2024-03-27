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

osc::TorusKnotGeometry::TorusKnotGeometry(
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

    const auto fNumTubularSegments = static_cast<float>(numTubularSegments);
    const auto fNumRadialSegments = static_cast<float>(numRadialSegments);
    const auto fp = static_cast<float>(p);
    const auto fq = static_cast<float>(q);

    // helper: calculates the current position on the torus curve
    const auto calculatePositionOnCurve = [fp, fq, torusRadius](Radians u)
    {
        const Radians quOverP = fq/fp * u;
        const float cs = cos(quOverP);

        return Vec3{
            torusRadius * (2.0f + cs) * 0.5f * cos(u),
            torusRadius * (2.0f + cs) * 0.5f * sin(u),
            torusRadius * sin(quOverP) * 0.5f,
        };
    };

    const size_t numVerts = (numTubularSegments+1)*(numRadialSegments+1);
    const size_t numIndices = 6*numTubularSegments*numRadialSegments;

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
        const auto fi = static_cast<float>(i);

        // `u` is used to calculate the position on the torus curve of the current tubular segment
        const Radians u = fi/fNumTubularSegments * fp * 360_deg;

        // now we calculate two points. P1 is our current position on the curve, P2 is a little farther ahead.
        // these points are used to create a special "coordinate space", which is necessary to calculate the
        // correct vertex positions
        const Vec3 P1 = calculatePositionOnCurve(u);
        const Vec3 P2 = calculatePositionOnCurve(u + 0.01_rad);

        // calculate orthonormal basis
        const Vec3 T = P2 - P1;
        Vec3 N = P2 + P1;
        Vec3 B = cross(T, N);
        N = cross(B, T);

        // normalize B, N. T can be ignored, we don't use it
        B = normalize(B);
        N = normalize(N);

        for (size_t j = 0; j <= numRadialSegments; ++j) {
            const auto fj = static_cast<float>(j);

            // now calculate the vertices. they are nothing more than an extrusion of the torus curve.
            // because we extrude a shape in the xy-plane, there is no need to calculate a z-value.

            const Radians v = fj/fNumRadialSegments * 360_deg;
            const float cx = -tubeRadius * cos(v);
            const float cy =  tubeRadius * sin(v);

            // now calculate the final vertex position.
            // first we orient the extrusion with our basis vectors, then we add it to the current position on the curve
            const Vec3 vertex = {
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
            const auto a = static_cast<uint32_t>((numRadialSegments + 1) * (j - 1) + (i - 1));
            const auto b = static_cast<uint32_t>((numRadialSegments + 1) *  j      + (i - 1));
            const auto c = static_cast<uint32_t>((numRadialSegments + 1) *  j      +  i);
            const auto d = static_cast<uint32_t>((numRadialSegments + 1) * (j - 1) +  i);

            indices.insert(indices.end(), {a, b, d});
            indices.insert(indices.end(), {b, c, d});
        }
    }

    // build geometry
    m_Mesh.setVerts(vertices);
    m_Mesh.setNormals(normals);
    m_Mesh.setTexCoords(uvs);
    m_Mesh.setIndices(indices);
}
