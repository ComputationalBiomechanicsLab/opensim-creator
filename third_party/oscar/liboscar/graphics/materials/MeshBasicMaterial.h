#pragma once

#include <liboscar/graphics/Color.h>
#include <liboscar/graphics/Material.h>
#include <liboscar/graphics/MaterialPropertyBlock.h>

#include <optional>

namespace osc
{
    struct MeshBasicMaterialParams final {
        friend bool operator==(const MeshBasicMaterialParams&, const MeshBasicMaterialParams&) = default;

        Color color = Color::black();
    };

    // a material for drawing meshes with a simple solid color
    class MeshBasicMaterial : public Material {
    public:
        using Params = MeshBasicMaterialParams;

        // a `MaterialPropertyBlock` that's specialized for the `MeshBasicMaterial`'s shader
        class PropertyBlock final : public MaterialPropertyBlock {
        public:
            static const StringName& color_property_name();

            explicit PropertyBlock() = default;
            explicit PropertyBlock(const Color&);

            std::optional<Color> color() const;
            void set_color(const Color&);
        };

        static const StringName& color_property_name();

        explicit MeshBasicMaterial(const Params& = {});
        explicit MeshBasicMaterial(const Color&);

        Color color() const;
        void set_color(const Color&);
    };
}
