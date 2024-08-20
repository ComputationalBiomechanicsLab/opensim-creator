#include "DodecahedronGeometry.h"

#include <oscar/Graphics/Geometries/PolyhedronGeometry.h>
#include <oscar/Graphics/Mesh.h>
#include <oscar/Maths/GeometricFunctions.h>

#include <array>
#include <cstddef>
#include <cstdint>

using namespace osc;

namespace
{
    PolyhedronGeometry as_polyhedron_geometry(const DodecahedronGeometry::Params& p)
    {
        // the implementation of this was initially translated from `three.js`'s
        // `DodecahedronGeometry`, which has excellent documentation and source code. The
        // code was then subsequently mutated to suit OSC, C++ etc.
        //
        // https://threejs.org/docs/#api/en/geometries/DodecahedronGeometry

        const float t = 0.5f * (1.0f + sqrt(5.0f));
        const float r = 1.0f/t;

        const auto vertices = std::to_array<Vec3>({
            {-1.0f, -1.0f, -1.0f}, {-1.0f, -1.0f,  1.0f},
            {-1.0f,  1.0f, -1.0f}, {-1.0f,  1.0f,  1.0f},
            { 1.0f, -1.0f, -1.0f}, { 1.0f, -1.0f,  1.0f},
            { 1.0f,  1.0f, -1.0f}, { 1.0f,  1.0f,  1.0f},

            { 0.0f, -r,    -t   }, { 0.0f, -r,     t   },
            { 0.0f,  r,    -t   }, { 0.0f,  r,     t   },

            {-r,    -t,     0.0f}, {-r,     t,     0.0f},
            { r,    -t,     0.0f}, { r,     t,     0.0f},

            {-t,     0.0f, -r   }, { t,     0.0f, -r   },
            {-t,     0.0f,  r   }, { t,     0.0f,  r   },
        });

        const auto indices = std::to_array<uint32_t>({
            3, 11, 7, 	3, 7, 15, 	3, 15, 13,
            7, 19, 17, 	7, 17, 6, 	7, 6, 15,
            17, 4, 8, 	17, 8, 10, 	17, 10, 6,
            8, 0, 16, 	8, 16, 2, 	8, 2, 10,
            0, 12, 1, 	0, 1, 18, 	0, 18, 16,
            6, 10, 2, 	6, 2, 13, 	6, 13, 15,
            2, 16, 18, 	2, 18, 3, 	2, 3, 13,
            18, 1, 9, 	18, 9, 11, 	18, 11, 3,
            4, 14, 12, 	4, 12, 0, 	4, 0, 8,
            11, 9, 5, 	11, 5, 19, 	11, 19, 7,
            19, 5, 14, 	19, 14, 4, 	19, 4, 17,
            1, 12, 14, 	1, 14, 5, 	1, 5, 9
        });

        return PolyhedronGeometry{vertices, indices, p.radius, p.detail};
    }
}

osc::DodecahedronGeometry::DodecahedronGeometry(const Params& p) :
    Mesh{as_polyhedron_geometry(p)}
{}
