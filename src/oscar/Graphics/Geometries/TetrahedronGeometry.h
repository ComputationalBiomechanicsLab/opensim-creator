#pragma once

#include <oscar/Graphics/Mesh.h>

#include <cstddef>

namespace osc
{
    class TetrahedronGeometry final {
    public:
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
