#pragma once

#include <oscar/Graphics/Material.h>

namespace osc
{
    // A `Material` that draws each of the `Mesh`'s vertex normals as lines
    // that originate at the vertex position and point in the direction of
    // the vertex's normal.
    class MeshNormalVectorsMaterial final : public Material {
    public:
        MeshNormalVectorsMaterial();
    };
}
