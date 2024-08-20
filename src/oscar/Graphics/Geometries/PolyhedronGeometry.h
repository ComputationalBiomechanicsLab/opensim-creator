#pragma once

#include <oscar/Graphics/Mesh.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/Utils/CStringView.h>

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace osc
{
    // parameters to construct `PolyhedronGeometry`
    //
    // defaults to a tetrahedron for demonstration purposes, callers should
    // overwrite `vertices` and `indices` as appropriate
    struct PolyhedronGeometryParams final {
        friend bool operator==(const PolyhedronGeometryParams&, const PolyhedronGeometryParams&);

        std::vector<Vec3> vertices = {
            {1.0f, 1.0f, 1.0f}, {-1.0f, -1.0f, 1.0f}, {-1.0f, 1.0f, -1.0f}, {1.0f, -1.0f, -1.0f},
        };
        std::vector<uint32_t> indices = {
            2, 1, 0,    0, 3, 2,    1, 3, 0,    2, 3, 1,
        };
        float radius = 1.0f;
        size_t detail_level = 0;
    };

    // generates a 3D solid with flat faces by projecting triangle faces (`indicies`
    // indexes into `vertices` for each triangle) onto a sphere of `radius`, followed
    // by dividing them up to the desired `detail_level`
    class PolyhedronGeometry final : public Mesh {
    public:
        using Params = PolyhedronGeometryParams;

        static constexpr CStringView name() { return "Polyhedron"; }

        explicit PolyhedronGeometry(const Params& = Params{});

        // constructs a `PolyhedronGeometry` from existing vertex + index data (rather
        // than requiring `std::vector`s)
        explicit PolyhedronGeometry(
            std::span<const Vec3> vertices,
            std::span<const uint32_t> indices,
            float radius = 1.0f,
            size_t detail_level = 0
        );
    };
}
