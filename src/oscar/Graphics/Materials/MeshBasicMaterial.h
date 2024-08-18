#pragma once

#include <oscar/Graphics/Material.h>
#include <oscar/Graphics/MaterialPropertyBlock.h>
#include <oscar/Graphics/Color.h>

#include <optional>
#include <string_view>

namespace osc
{
    // a material for drawing meshes with a simple solid color
    class MeshBasicMaterial final : public Material {
    public:
        class PropertyBlock final {
        public:
            PropertyBlock() = default;
            explicit PropertyBlock(Color color)
            {
                property_block_.set<Color>(c_color_propname, color);
            }

            std::optional<Color> color() const { return property_block_.get<Color>(c_color_propname); }
            void set_color(const Color& c) { property_block_.set(c_color_propname, c); }

            operator const MaterialPropertyBlock& () const { return property_block_; }
        private:
            MaterialPropertyBlock property_block_;
        };

        explicit MeshBasicMaterial(const Color& = Color::black());

        Color color() const { return *get<Color>(c_color_propname); }
        void set_color(const Color& color) { set(c_color_propname, color); }

    private:
        static const StringName c_color_propname;
    };
}
