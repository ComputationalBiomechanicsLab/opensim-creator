#pragma once

#include <oscar/Graphics/Mesh.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/Utils/CStringView.h>

#include <cstddef>
#include <cstdint>
#include <span>

namespace osc
{
    // generates a 3D solid with flat faces by projecting triangle faces (`indicies`
    // indexes into `vertices` for each triangle) onto a sphere of `radius`, followed
    // by dividing them up to the desired `detail_level`
    class PolyhedronGeometry final {
    public:
        static constexpr CStringView name() { return "Polyhedron"; }

        PolyhedronGeometry(
            std::span<const Vec3> vertices,
            std::span<const uint32_t> indices,
            float radius,
            size_t detail_level
        );

        operator const Mesh& () const { return mesh_; }
        const Mesh& mesh() const { return mesh_; }
    private:
        Mesh mesh_;
    };
}
