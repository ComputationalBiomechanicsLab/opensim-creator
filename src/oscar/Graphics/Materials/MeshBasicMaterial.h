#pragma once

#include <oscar/Graphics/Material.h>
#include <oscar/Graphics/MaterialPropertyBlock.h>
#include <oscar/Graphics/Color.h>

#include <optional>

namespace osc
{
    struct MeshBasicMaterialParams final {
        friend bool operator==(const MeshBasicMaterialParams&, const MeshBasicMaterialParams&) = default;

        Color color = Color::black();
    };

    // a material for drawing meshes with a simple solid color
    class MeshBasicMaterial final : public Material {
    public:
        using Params = MeshBasicMaterialParams;

        // a `MaterialPropertyBlock` that's specialized for the `MeshBasicMaterial`'s shader
        class PropertyBlock final : public MaterialPropertyBlock {
        public:
            explicit PropertyBlock() = default;

            explicit PropertyBlock(const Color&);

            std::optional<Color> color() const;
            void set_color(const Color&);
        };

        explicit MeshBasicMaterial(const Params& = {});
        explicit MeshBasicMaterial(const Color&);

        Color color() const;
        void set_color(const Color&);
    };
}
