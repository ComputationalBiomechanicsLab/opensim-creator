#include "TetrahedronGeometry.h"

#include <oscar/Graphics/Geometries/PolyhedronGeometry.h>
#include <oscar/Graphics/Mesh.h>

#include <array>
#include <cstdint>

using namespace osc;

namespace
{
    PolyhedronGeometry as_polyhedron_geometry(const TetrahedronGeometry::Params& p)
    {
        // the implementation of this was initially translated from `three.js`'s
        // `TetrahedronGeometry`, which has excellent documentation and source code. The
        // code was then subsequently mutated to suit OSC, C++ etc.
        //
        // https://threejs.org/docs/#api/en/geometries/TetrahedronGeometry

        constexpr auto vertices = std::to_array<Vec3>({
            {1.0f, 1.0f, 1.0f}, {-1.0f, -1.0f, 1.0f}, {-1.0f, 1.0f, -1.0f}, {1.0f, -1.0f, -1.0f},
        });

        constexpr auto indices = std::to_array<uint32_t>({
            2, 1, 0,    0, 3, 2,    1, 3, 0,    2, 3, 1
        });

        return PolyhedronGeometry{vertices, indices, p.radius, p.detail_level};
    }
}

osc::TetrahedronGeometry::TetrahedronGeometry(const Params& p) :
    Mesh{as_polyhedron_geometry(p)}
{}
