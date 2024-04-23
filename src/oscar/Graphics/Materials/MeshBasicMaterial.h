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
                property_block_.set_color(c_color_propname, color);
            }

            std::optional<Color> color() const { return property_block_.get_color(c_color_propname); }
            void set_color(Color c) { property_block_.set_color(c_color_propname, c); }

            operator const MaterialPropertyBlock& () const { return property_block_; }
        private:
            MaterialPropertyBlock property_block_;
        };

        MeshBasicMaterial();

        Color color() const { return *material_.get_color(c_color_propname); }
        void set_color(Color c) { material_.set_color(c_color_propname, c); }

        bool is_wireframe() const { return material_.is_wireframe(); }
        void set_wireframe(bool v) { material_.set_wireframe(v); }

        bool is_depth_tested() const { return material_.is_depth_tested(); }
        void set_depth_tested(bool v) { material_.set_depth_tested(v); }

        bool is_transparent() const { return material_.is_transparent(); }
        void set_transparent(bool v) { material_.set_transparent(v); }

        operator const Material& () const { return material_; }

    private:
        static constexpr std::string_view c_color_propname = "uDiffuseColor";
        Material material_;
    };
}
