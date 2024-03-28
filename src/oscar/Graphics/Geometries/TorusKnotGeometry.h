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
        TorusKnotGeometry(
            float torus_radius = 1.0f,
            float tube_radius = 0.4f,
            size_t num_tubular_segments = 64,
            size_t num_radial_segments = 8,
            size_t p = 2,
            size_t q = 3
        );

        const Mesh& mesh() const { return mesh_; }
        operator const Mesh& () const { return mesh_; }
    private:
        Mesh mesh_;
    };
}
