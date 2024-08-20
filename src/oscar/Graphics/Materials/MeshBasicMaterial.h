#pragma once

#include <oscar/Graphics/Material.h>
#include <oscar/Graphics/MaterialPropertyBlock.h>
#include <oscar/Graphics/Color.h>

#include <optional>
#include <string_view>

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

            explicit PropertyBlock(const Color& color)
            {
                set_color(color);
            }

            std::optional<Color> color() const { return get<Color>(c_color_propname); }
            void set_color(const Color& c) { set(c_color_propname, c); }
        };

        explicit MeshBasicMaterial(const Params& = {});
        explicit MeshBasicMaterial(const Color& color) : MeshBasicMaterial{Params{.color = color}} {}

        Color color() const { return *get<Color>(c_color_propname); }
        void set_color(const Color& color) { set(c_color_propname, color); }

    private:
        static const StringName c_color_propname;
    };
}
