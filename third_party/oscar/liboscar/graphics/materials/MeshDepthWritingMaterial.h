#pragma once

#include <liboscar/graphics/Material.h>

namespace osc
{
    // A material that only writes the depth of the mesh to the depth buffer (no
    // color output)
    class MeshDepthWritingMaterial final : public Material {
    public:
        MeshDepthWritingMaterial();
    };
}
