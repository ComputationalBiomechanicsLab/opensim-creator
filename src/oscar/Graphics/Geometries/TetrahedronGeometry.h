#pragma once

#include <oscar/Graphics/Mesh.h>
#include <oscar/Utils/CStringView.h>

#include <cstddef>

namespace osc
{
    class TetrahedronGeometry final {
    public:
        static constexpr CStringView name() { return "Tetrahedron"; }

        TetrahedronGeometry(
            float radius = 1.0f,
            size_t detail_level = 0
        );

        const Mesh& mesh() const { return mesh_; }
        operator const Mesh& () const { return mesh_; }
    private:
        Mesh mesh_;
    };
}
