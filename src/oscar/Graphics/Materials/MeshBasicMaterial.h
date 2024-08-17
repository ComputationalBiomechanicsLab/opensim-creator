#pragma once

#include <oscar/Graphics/Material.h>
#include <oscar/Graphics/MaterialPropertyBlock.h>
#include <oscar/Graphics/Color.h>

#include <optional>
#include <string_view>

namespace osc
{
    // a material for drawing meshes with a simple solid color
    class MeshBasicMaterial final {
    public:
        class PropertyBlock final {
        public:
            PropertyBlock() = default;
            explicit PropertyBlock(Color color)
            {
                property_block_.set<Color>(c_color_propname, color);
            }

            std::optional<Color> color() const { return property_block_.get<Color>(c_color_propname); }
            void set_color(Color c) { property_block_.set<Color>(c_color_propname, c); }

            operator const MaterialPropertyBlock& () const { return property_block_; }
        private:
            MaterialPropertyBlock property_block_;
        };

        explicit MeshBasicMaterial(const Color& = Color::black());

        Color color() const { return *material_.get<Color>(c_color_propname); }
        void set_color(Color c) { material_.set<Color>(c_color_propname, c); }

        bool is_wireframe() const { return material_.is_wireframe(); }
        void set_wireframe(bool v) { material_.set_wireframe(v); }

        bool is_depth_tested() const { return material_.is_depth_tested(); }
        void set_depth_tested(bool v) { material_.set_depth_tested(v); }

        bool is_transparent() const { return material_.is_transparent(); }
        void set_transparent(bool v) { material_.set_transparent(v); }

        operator const Material& () const { return material_; }

    private:
        static const StringName c_color_propname;
        Material material_;
    };
}
