#include "OctahedronGeometry.h"

#include <liboscar/Graphics/Geometries/PolyhedronGeometry.h>
#include <liboscar/Graphics/Mesh.h>

#include <array>
#include <cstdint>

using namespace osc;

namespace
{
    PolyhedronGeometry as_polyhedron_geometry(const OctahedronGeometry::Params& p)
    {
        // the implementation of this was initially translated from `three.js`'s
        // `OctahedronGeometry`, which has excellent documentation and source code. The
        // code was then subsequently mutated to suit OSC, C++ etc.
        //
        // https://threejs.org/docs/#api/en/geometries/OctahedronGeometry

        constexpr auto vertices = std::to_array<Vec3>({
            {1.0f,  0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f,  0.0f},
            {0.0f, -1.0f, 0.0f}, { 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, -1.0f},
        });

        constexpr auto indices = std::to_array<uint32_t>({
            0, 2, 4,    0, 4, 3,    0, 3, 5,
            0, 5, 2,    1, 2, 5,    1, 5, 3,
            1, 3, 4,    1, 4, 2
        });

        return PolyhedronGeometry{vertices, indices, p.radius, p.detail};
    }
}

osc::OctahedronGeometry::OctahedronGeometry(const Params& p) :
    Mesh{as_polyhedron_geometry(p)}
{}
