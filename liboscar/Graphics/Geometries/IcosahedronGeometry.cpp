#include "IcosahedronGeometry.h"

#include <liboscar/Graphics/Geometries/PolyhedronGeometry.h>
#include <liboscar/Graphics/Mesh.h>
#include <liboscar/Maths/GeometricFunctions.h>

#include <array>
#include <cstdint>

using namespace osc;

namespace
{
    PolyhedronGeometry as_polyhedron_geometry(const IcosahedronGeometry::Params& p)
    {
        // the implementation of this was initially translated from `three.js`'s
        // `IcosahedronGeometry`, which has excellent documentation and source code. The
        // code was then subsequently mutated to suit OSC, C++ etc.
        //
        // https://threejs.org/docs/#api/en/geometries/IcosahedronGeometry

        const float t = 0.5f * (1.0f + sqrt(5.0f));

        const auto vertices = std::to_array<Vec3>({
            {-1.0f,  t,     0.0f}, {1.0f, t,    0.0f}, {-1.0f, -t,     0.0f}, { 1.0f, -t,     0.0f},
            { 0.0f, -1.0f,  t   }, {0.0f, 1.0f, t   }, { 0.0f, -1.0f, -t   }, { 0.0f,  1.0f, -t   },
            { t,     0.0f, -1.0f}, {t,    0.0f, 1.0f}, {-t,     0.0f, -1.0f}, {-t,     0.0f,  1.0f},
        });

        const auto indices = std::to_array<uint32_t>({
            0, 11, 5,    0, 5,  1,     0,  1,  7,     0,  7, 10,    0, 10, 11,
            1, 5,  9,    5, 11, 4,     11, 10, 2,     10, 7, 6,     7, 1,  8,
            3, 9,  4,    3, 4,  2,     3,  2,  6,     3,  6, 8,     3, 8, 9,
            4, 9,  5,    2, 4,  11,    6,  2,  10,    8,  6, 7,     9, 8, 1,
        });

        return PolyhedronGeometry{vertices, indices, p.radius, p.detail};
    }
}

osc::IcosahedronGeometry::IcosahedronGeometry(const Params& p) :
    Mesh{as_polyhedron_geometry(p)}
{}
