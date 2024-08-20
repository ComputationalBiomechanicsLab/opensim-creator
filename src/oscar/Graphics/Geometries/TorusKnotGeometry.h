#pragma once

#include <oscar/Graphics/Mesh.h>
#include <oscar/Utils/CStringView.h>

#include <cstddef>

namespace osc
{
    struct TorusKnotGeometryParams final {
        friend bool operator==(const TorusKnotGeometryParams&, const TorusKnotGeometryParams&) = default;

        float torus_radius = 1.0f;
        float tube_radius = 0.4f;
        size_t num_tubular_segments = 64;
        size_t num_radial_segments = 8;
        size_t p = 2;
        size_t q = 3;
    };

    // generates a torus knot, the particular shape of which is defined by a pair
    // of coprime integers `p` and `q`. If `p` and `q` are not coprime, the result
    // will be a torus link
    class TorusKnotGeometry final : public Mesh {
    public:
        using Params = TorusKnotGeometryParams;

        static constexpr CStringView name() { return "Torus Knot"; }

        explicit TorusKnotGeometry(const Params& = {});
    };
}
