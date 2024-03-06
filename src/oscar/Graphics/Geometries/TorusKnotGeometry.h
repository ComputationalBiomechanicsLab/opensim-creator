#pragma once

#include <oscar/Graphics/Mesh.h>

#include <cstddef>

namespace osc
{
    // generates a torus knot, the particular shape of which is defined by a pair of coprime integers
    // `p` and `q`. If `p` and `q` are not coprime, the result will be a torus link
    //
    // inspired by three.js's `TorusKnotGeometry`
    class TorusKnotGeometry final {
    public:
        static Mesh generate_mesh(
            float torusRadius = 1.0f,
            float tubeRadius = 0.4f,
            size_t numTubularSegments = 64,
            size_t numRadialSegments = 8,
            size_t p = 2,
            size_t q = 3
        );
    };
}
