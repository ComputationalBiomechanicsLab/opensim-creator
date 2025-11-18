#pragma once

#include <liboscar/Graphics/Material.h>
#include <liboscar/Graphics/Texture2D.h>

namespace osc
{
    struct MeshBasicTexturedMaterialParams final {
        friend bool operator==(const MeshBasicTexturedMaterialParams&, const MeshBasicTexturedMaterialParams&) = default;

        Texture2D texture;
    };

    // a material for drawing meshes with a simple solid color
    class MeshBasicTexturedMaterial : public Material {
    public:
        using Params = MeshBasicTexturedMaterialParams;

        explicit MeshBasicTexturedMaterial(const Params& = {});
        explicit MeshBasicTexturedMaterial(Texture2D texture) :
            MeshBasicTexturedMaterial{Params{.texture = texture}}
        {}

        Texture2D texture() const;
        void set_texture(const Texture2D&);
    };
}
